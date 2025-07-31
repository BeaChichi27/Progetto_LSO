#include "headers/lobby.h"
#include "headers/network.h"
#include "headers/game_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Client *clients[MAX_CLIENTS];
static char registered_names[MAX_CLIENTS][MAX_NAME_LEN];
static int name_count = 0;


int lobby_init() {
    InitializeCriticalSection(&lobby_mutex);
    InitializeCriticalSection(&names_mutex);
    EnterCriticalSection(&lobby_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = NULL;
    }
    LeaveCriticalSection(&lobby_mutex);
    printf("Lobby inizializzata\n");
    return 1;
}

void lobby_cleanup() {
    EnterCriticalSection(&lobby_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            game_leave(clients[i]);
            clients[i] = NULL;
        }
    }
    LeaveCriticalSection(&lobby_mutex);
    DeleteCriticalSection(&lobby_mutex);
    printf("Lobby pulita\n");
}

static int lobby_find_free_slot() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == NULL) {
            return i;
        }
    }
    return -1;
}

Client* lobby_add_client(SOCKET client_fd, const char *name) {
    EnterCriticalSection(&lobby_mutex);
    int slot = lobby_find_free_slot();
    if (slot == -1) {
        LeaveCriticalSection(&lobby_mutex);
        return NULL;
    }
    Client *client = (Client*)malloc(sizeof(Client));
    if (!client) {
        LeaveCriticalSection(&lobby_mutex);
        return NULL;
    }
    memset(client, 0, sizeof(Client));
    client->client_fd = client_fd;
    strncpy(client->name, name, MAX_NAME_LEN - 1);
    client->name[MAX_NAME_LEN - 1] = '\0';
    client->is_active = 1;
    client->game_id = -1;
    clients[slot] = client;
    LeaveCriticalSection(&lobby_mutex);
    printf("Client %s aggiunto alla lobby\n", name);
    return client;
}


void lobby_remove_client(Client *client) {
    if (!client) return;
    
    EnterCriticalSection(&lobby_mutex);
    
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client) {
            
            if (strlen(client->name) > 0) {
                remove_name(client->name);
            }
            
            
            closesocket(client->client_fd);
            free(client);
            clients[i] = NULL;
            break;
        }
    }
    
    LeaveCriticalSection(&lobby_mutex);
    
    printf("Client rimosso dalla lobby\n");
}

Client* lobby_find_client_by_fd(SOCKET fd) {
    EnterCriticalSection(&lobby_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->client_fd == fd) {
            Client *found = clients[i];
            LeaveCriticalSection(&lobby_mutex);
            return found;
        }
    }
    LeaveCriticalSection(&lobby_mutex);
    return NULL;
}

Client* lobby_find_client_by_name(const char *name) {
    if (!name) return NULL;
    EnterCriticalSection(&lobby_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && strcmp(clients[i]->name, name) == 0) {
            Client *found = clients[i];
            LeaveCriticalSection(&lobby_mutex);
            return found;
        }
    }
    LeaveCriticalSection(&lobby_mutex);
    return NULL;
}

void lobby_broadcast_message(const char *message, Client *exclude) {
    if (!message) return;
    EnterCriticalSection(&lobby_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i] != exclude && clients[i]->is_active) {
            network_send_to_client(clients[i], message);
        }
    }
    LeaveCriticalSection(&lobby_mutex);
}



void handle_registration(Client *client, const char *name) {
    EnterCriticalSection(&lobby_mutex);
    EnterCriticalSection(&names_mutex);
    if (!client || !name || strlen(name) == 0) {
        network_send_to_client(client, "ERROR: Nome non valido");
        LeaveCriticalSection(&names_mutex);
        LeaveCriticalSection(&lobby_mutex);
        return;
    }

    int is_duplicate = is_name_duplicate(name);
    
    if (!is_duplicate) {
        add_name(name);
        strncpy(client->name, name, MAX_NAME_LEN - 1);
        client->name[MAX_NAME_LEN - 1] = '\0';
        network_send_to_client(client, "REGISTRATION_OK");
    } else {
        network_send_to_client(client, "ERROR: Nome già in uso");
        client->is_active = 0; 
    }
    LeaveCriticalSection(&names_mutex);
    LeaveCriticalSection(&lobby_mutex);
}


void lobby_handle_client_message(Client *client, const char *message) {
    if (!client || !message) return;
    
    char cleaned_msg[MAX_MSG_SIZE];
    strncpy(cleaned_msg, message, sizeof(cleaned_msg) - 1);
    cleaned_msg[sizeof(cleaned_msg) - 1] = '\0';
    
    char *newline = strchr(cleaned_msg, '\n');
    if (newline) *newline = '\0';
    
    printf("Messaggio da %s: '%s'\n", client->name, cleaned_msg);
    
    if (strncmp(cleaned_msg, "REGISTER:", 9) == 0) {
        const char *name = cleaned_msg + 9;
        handle_registration(client, name);
    } else if (strncmp(cleaned_msg, "CREATE_GAME", 11) == 0) {
        lobby_handle_create_game(client);
    } else if (strncmp(cleaned_msg, "JOIN:", 5) == 0) {
        int game_id = atoi(cleaned_msg + 5);
        lobby_handle_join_game(client, game_id);
    } else if (strncmp(cleaned_msg, "LIST_GAMES", 10) == 0) {
        lobby_handle_list_games(client);
    } else if (strncmp(cleaned_msg, "MOVE:", 5) == 0) {
        int row, col;
        if (sscanf(cleaned_msg + 5, "%d,%d", &row, &col) == 2) {
            lobby_handle_move(client, row, col);
        }
    } else if (strncmp(cleaned_msg, "REMATCH", 7) == 0) {
        lobby_handle_rematch(client);
    } else if (strncmp(cleaned_msg, "HEARTBEAT_ACK", 13) == 0) {
        client->last_heartbeat_ack = time(NULL);
    } else {
        network_send_to_client(client, "ERROR: Comando sconosciuto");
    }
}

void lobby_handle_create_game(Client *client) {
    if (!client) return;
    
    int game_id = game_create_new(client);
    if (game_id > 0) {
        char response[50];
        sprintf(response, "GAME_CREATED:%d", game_id);
        network_send_to_client(client, response);
    } else {
        network_send_to_client(client, "ERROR:Impossibile creare la partita");
    }
}

void lobby_handle_join_game(Client *client, int game_id) {
    if (!client || game_id <= 0) {
        network_send_to_client(client, "ERROR:ID partita non valido");
        return;
    }
    
    if (game_join(client, game_id)) {
        network_send_to_client(client, "JOIN_SUCCESS");
    } else {
        network_send_to_client(client, "ERROR:Impossibile unirsi alla partita");
    }
}

void lobby_handle_list_games(Client *client) {
    if (!client) return;
    
    char response[MAX_MSG_SIZE];
    game_list_available(response, sizeof(response));
    network_send_to_client(client, response);
}

void lobby_handle_move(Client *client, int row, int col) {
    if (!client || client->game_id <= 0) {
        network_send_to_client(client, "ERROR:Non sei in una partita");
        return;
    }
    
    if (game_make_move(client->game_id, client, row, col)) {
        network_send_to_client(client, "MOVE_ACCEPTED");
    } else {
        network_send_to_client(client, "ERROR:Mossa non valida");
    }
}

void lobby_handle_rematch(Client *client) {
    if (!client || client->game_id <= 0) {
        network_send_to_client(client, "ERROR:Non sei in una partita");
        return;
    }
    
    if (game_request_rematch(client->game_id, client)) {
        network_send_to_client(client, "REMATCH_REQUEST_SENT");
    } else {
        network_send_to_client(client, "ERROR:Impossibile richiedere la rivincita");
    }
}

void lobby_check_timeouts() {
    EnterCriticalSection(&lobby_mutex);
    time_t now = time(NULL);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->is_active) {
            if (difftime(now, clients[i]->last_heartbeat_ack) > 15) {
                printf("Client %s disconnesso per timeout\n", clients[i]->name);
                closesocket(clients[i]->client_fd);
                clients[i]->is_active = 0;
            }
        }
    }
    LeaveCriticalSection(&lobby_mutex);
}

CRITICAL_SECTION* lobby_get_mutex() {
    return &lobby_mutex;
}


Client* lobby_get_client_by_index(int index) {
    if (index < 0 || index >= MAX_CLIENTS) return NULL;
    return clients[index]; 
}

int lobby_add_client_reference(Client *client) {
    if (!client) return 0;
    CRITICAL_SECTION* mutex = lobby_get_mutex();
    EnterCriticalSection(mutex);
    
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == NULL) {
            clients[i] = client;
            LeaveCriticalSection(mutex);
            return 1;
        }
    }
    
    LeaveCriticalSection(mutex);
    return 0; 
}


int is_name_duplicate(const char *name) {
    EnterCriticalSection(&lobby_mutex);
    for (int i = 0; i < name_count; i++) {
        if (strcmp(registered_names[i], name) == 0) {
            LeaveCriticalSection(&lobby_mutex);
            return 1;
        }
    }
    LeaveCriticalSection(&lobby_mutex);
    return 0;
}

void remove_name(const char *name) {
    EnterCriticalSection(&lobby_mutex);
    for (int i = 0; i < name_count; i++) {
        if (strcmp(registered_names[i], name) == 0) {
            for (int j = i; j < name_count - 1; j++) {
                strcpy(registered_names[j], registered_names[j + 1]);
            }
            name_count--;
            break;
        }
    }
    LeaveCriticalSection(&lobby_mutex);
}

void add_name(const char *name) {
    EnterCriticalSection(&names_mutex);
    if (name_count < MAX_CLIENTS && !is_name_duplicate(name)) {
        strncpy(registered_names[name_count], name, MAX_NAME_LEN - 1);
        registered_names[name_count][MAX_NAME_LEN - 1] = '\0';
        name_count++;
    }
    LeaveCriticalSection(&names_mutex);
}