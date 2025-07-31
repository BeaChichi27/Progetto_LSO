#include "headers/network.h"
#include "headers/lobby.h"
#include "headers/game_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ServerNetwork *global_server = NULL;

static int network_register_udp_client(Client *client, struct sockaddr_in *udp_addr) {
    if (!client || !udp_addr) return 0;
    
    
    memcpy(&client->udp_addr, udp_addr, sizeof(struct sockaddr_in));
    
    printf("Client %s registrato per UDP da %s:%d\n", 
           client->name, 
           inet_ntoa(udp_addr->sin_addr), 
           ntohs(udp_addr->sin_port));
    return 1;
}


static Client* network_find_client_by_udp_addr(struct sockaddr_in *addr) {
    if (!addr) return NULL;
    
    CRITICAL_SECTION* mutex = lobby_get_mutex();
    EnterCriticalSection(mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        Client *client = lobby_get_client_by_index(i); 
        if (client && 
            client->udp_addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
            client->udp_addr.sin_port == addr->sin_port) {
            LeaveCriticalSection(mutex);
            return client;
        }
    }
    
    LeaveCriticalSection(mutex);
    return NULL;
}


static int network_send_udp_response(ServerNetwork *server, struct sockaddr_in *client_addr, const char *message) {
    if (!server || !client_addr || !message) return 0;
    
    int bytes_sent = sendto(server->udp_socket, message, (int)strlen(message), 0,
                           (struct sockaddr*)client_addr, sizeof(*client_addr));
    
    if (bytes_sent == SOCKET_ERROR) {
        printf("Errore invio UDP: %d\n", WSAGetLastError());
        return 0;
    }
    
    printf("UDP inviato a %s:%d: %s\n", 
           inet_ntoa(client_addr->sin_addr), 
           ntohs(client_addr->sin_port), 
           message);
    return 1;
}



static void handle_udp_register(ServerNetwork *server, struct sockaddr_in *client_addr, const char *name) {
    
    Client *client = lobby_find_client_by_name(name);
    if (!client) {
        network_send_udp_response(server, client_addr, "ERROR:Client non trovato");
        return;
    }
    
    
    if (network_register_udp_client(client, client_addr)) {
        network_send_udp_response(server, client_addr, "UDP_REGISTERED:OK");
    } else {
        network_send_udp_response(server, client_addr, "ERROR:Registrazione fallita");
    }
}

static void handle_udp_move(ServerNetwork *server, struct sockaddr_in *client_addr, const char *move_data) {
    int row, col;
    if (sscanf(move_data, "%d,%d", &row, &col) != 2) {
        network_send_udp_response(server, client_addr, "ERROR:Formato mossa non valido");
        return;
    }
    
    
    Client *client = network_find_client_by_udp_addr(client_addr);
    if (!client) {
        network_send_udp_response(server, client_addr, "ERROR:Client non registrato per UDP");
        return;
    }
    
    
    if (client->game_id <= 0) {
        network_send_udp_response(server, client_addr, "ERROR:Non sei in una partita");
        return;
    }
    
    
    if (game_make_move(client->game_id, client, row, col)) {
        network_send_udp_response(server, client_addr, "MOVE_ACCEPTED");
    } else {
        network_send_udp_response(server, client_addr, "ERROR:Mossa non valida");
    }
}

static void handle_udp_ping(ServerNetwork *server, struct sockaddr_in *client_addr) {
    network_send_udp_response(server, client_addr, "PONG");
}

static void handle_udp_game_state(ServerNetwork *server, struct sockaddr_in *client_addr) {
    Client *client = network_find_client_by_udp_addr(client_addr);
    if (!client) {
        network_send_udp_response(server, client_addr, "ERROR:Client non registrato");
        return;
    }
    
    if (client->game_id <= 0) {
        network_send_udp_response(server, client_addr, "GAME_STATE:NO_GAME");
        return;
    }
    
    Game *game = game_find_by_id(client->game_id);
    if (!game) {
        network_send_udp_response(server, client_addr, "ERROR:Partita non trovata");
        return;
    }
    
    
    char state_msg[256];
    sprintf(state_msg, "GAME_STATE:%d:%c:%d", 
            game->game_id, 
            (char)game->current_player, 
            (int)game->state);
    
    network_send_udp_response(server, client_addr, state_msg);
}



DWORD WINAPI network_handle_udp_thread(LPVOID arg) {
    ServerNetwork *server = (ServerNetwork*)arg;
    char buffer[MAX_MSG_SIZE];
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    
    printf("Thread UDP avviato\n");
    
    while (server->is_running) {
        Sleep(5000);
        lobby_broadcast_message("HEARTBEAT", NULL);

        int bytes = recvfrom(server->udp_socket, buffer, sizeof(buffer) - 1, 0,
                           (struct sockaddr*)&client_addr, &addr_len);
        
        if (bytes <= 0) {
            int error = WSAGetLastError();
            if (error == WSAETIMEDOUT) {
                continue;
            }
            if (server->is_running) {
                printf("Errore ricezione UDP: %d\n", error);
            }
            continue;
        }

        
        buffer[bytes] = '\0';
        
        printf("UDP ricevuto: %s da %s:%d\n", 
               buffer, 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));
        
        
        if (strncmp(buffer, "UDP_REGISTER:", 13) == 0) {
            const char *name = buffer + 13;
            handle_udp_register(server, &client_addr, name);
        }
        else if (strncmp(buffer, "MOVE:", 5) == 0) {
            const char *move_data = buffer + 5;
            handle_udp_move(server, &client_addr, move_data);
        }
        else if (strncmp(buffer, "PING", 4) == 0) {
            handle_udp_ping(server, &client_addr);
        }
        else if (strncmp(buffer, "GET_GAME_STATE", 14) == 0) {
            handle_udp_game_state(server, &client_addr);
        }
        else if (strncmp(buffer, "UDP_DISCONNECT", 14) == 0) {
            Client *client = network_find_client_by_udp_addr(&client_addr);
            if (client) {
                memset(&client->udp_addr, 0, sizeof(client->udp_addr));
                network_send_udp_response(server, &client_addr, "UDP_DISCONNECTED");
                printf("Client %s disconnesso da UDP\n", client->name);
            }
        }
        else {
            network_send_udp_response(server, &client_addr, "ERROR:Comando UDP sconosciuto");
        }
    }
    
    printf("Thread UDP terminato\n");
    return 0;
}



int network_init(ServerNetwork *server) {
    WSADATA wsaData;
    long result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup fallito: %lu\n", result);
        return 0;
    }

    memset(server, 0, sizeof(ServerNetwork));
    
    
    server->tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server->tcp_socket == INVALID_SOCKET) {
        printf("Errore creazione socket TCP: %d\n", WSAGetLastError());
        WSACleanup();
        return 0;
    }
    
    
    server->udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server->udp_socket == INVALID_SOCKET) {
        printf("Errore creazione socket UDP: %d\n", WSAGetLastError());
        closesocket(server->tcp_socket);
        WSACleanup();
        return 0;
    }
    
    
    memset(&server->server_addr, 0, sizeof(server->server_addr));
    server->server_addr.sin_family = AF_INET;
    server->server_addr.sin_addr.s_addr = INADDR_ANY;
    server->server_addr.sin_port = htons(SERVER_PORT);
    
    
    BOOL opt = TRUE;
    setsockopt(server->tcp_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    setsockopt(server->udp_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    
    
    DWORD timeout = 3000; 
    setsockopt(server->udp_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    global_server = server;
    return 1;
}

int network_start_listening(ServerNetwork *server) {
    
    if (bind(server->tcp_socket, (struct sockaddr*)&server->server_addr, 
             sizeof(server->server_addr)) == SOCKET_ERROR) {
        printf("Errore bind TCP: %d\n", WSAGetLastError());
        return 0;
    }
    
    
    if (bind(server->udp_socket, (struct sockaddr*)&server->server_addr, 
             sizeof(server->server_addr)) == SOCKET_ERROR) {
        printf("Errore bind UDP: %d\n", WSAGetLastError());
        return 0;
    }
    
    
    if (listen(server->tcp_socket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Errore listen TCP: %d\n", WSAGetLastError());
        return 0;
    }
    
    server->is_running = 1;
    printf("Server in ascolto sulla porta %d (TCP e UDP)\n", SERVER_PORT);
    
    
    HANDLE udp_thread = CreateThread(NULL, 0, network_handle_udp_thread, server, 0, NULL);
    if (udp_thread == NULL) {
        printf("Errore creazione thread UDP: %lu\n", GetLastError());
        return 0;
    }
    CloseHandle(udp_thread);
    
    return 1;
}

void network_shutdown(ServerNetwork *server) {
    server->is_running = 0;
    
    if (server->tcp_socket != INVALID_SOCKET) {
        closesocket(server->tcp_socket);
        server->tcp_socket = INVALID_SOCKET;
    }
    
    if (server->udp_socket != INVALID_SOCKET) {
        closesocket(server->udp_socket);
        server->udp_socket = INVALID_SOCKET;
    }
    
    WSACleanup();
    printf("Server spento\n");
}



Client* network_accept_client(ServerNetwork *server) {
    struct sockaddr_in client_addr;
    int client_len = sizeof(client_addr);
    
    SOCKET client_fd = accept(server->tcp_socket, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == INVALID_SOCKET) {
        int error = WSAGetLastError();
        if (error != WSAEINTR && server->is_running) {
            printf("Errore accept: %d\n", error);
        }
        return NULL;
    }
    
    printf("Nuova connessione TCP da %s:%d\n", 
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    Client *client = (Client*)malloc(sizeof(Client));
    if (!client) {
        closesocket(client_fd);
        return NULL;
    }
    
    memset(client, 0, sizeof(Client));
    client->client_fd = client_fd;
    client->is_active = 1;
    client->game_id = -1;
    strcpy(client->name, "Unknown");
    
    return client;
}

int network_send_to_client(Client *client, const char *message) {
    if (!client || !client->is_active || !message) {
        return 0;
    }

    EnterCriticalSection(&lobby_mutex); 
    
    int bytes_sent = send(client->client_fd, message, (int)strlen(message), 0);
    if (bytes_sent == SOCKET_ERROR) {
        int error = WSAGetLastError();
        printf("Errore invio a %s: %d\n", client->name, error);
        client->is_active = 0;
        LeaveCriticalSection(&lobby_mutex);
        return 0;
    }

    printf("Inviato a %s: %s\n", client->name, message);
    LeaveCriticalSection(&lobby_mutex);
    return 1;
}

int network_receive_from_client(Client *client, char *buffer, size_t buf_size) {
    if (!client || !client->is_active || !buffer) {
        return -1;
    }
    
    int bytes = recv(client->client_fd, buffer, (int)buf_size - 1, 0);
    if (bytes <= 0) {
        if (bytes == 0) {
            printf("Client %s disconnesso\n", client->name);
        } else {
            printf("Errore ricezione messaggio: %d\n", WSAGetLastError());
        }
        client->is_active = 0;
        return -1;
    }
    
    buffer[bytes] = '\0';
    printf("TCP ricevuto da %s: %s\n", client->name, buffer);
    return bytes;
}

DWORD WINAPI network_handle_client_thread(LPVOID arg) {
    Client *client = (Client*)arg;
    char buffer[MAX_MSG_SIZE];

    
    while (client->is_active && global_server->is_running) {
        int bytes = network_receive_from_client(client, buffer, sizeof(buffer));
        if (bytes <= 0) {
            goto cleanup;
        }

        if (strncmp(buffer, "REGISTER:", 9) == 0) {
            const char *name = buffer + 9;

            
            EnterCriticalSection(&lobby_mutex);
            if (strlen(name) == 0) {
                network_send_to_client(client, "ERROR:Nome non valido");
                LeaveCriticalSection(&lobby_mutex);
                continue; 
            }

            if (is_name_duplicate(name)) {
                network_send_to_client(client, "ERROR:Nome già in uso");
                LeaveCriticalSection(&lobby_mutex);
                continue; 
            }

            add_name(name);
            strncpy(client->name, name, MAX_NAME_LEN - 1);
            client->name[MAX_NAME_LEN - 1] = '\0';
            LeaveCriticalSection(&lobby_mutex);

            network_send_to_client(client, "REGISTRATION_OK");
            break; 
        }
    }

    
    if (strlen(client->name) == 0) {
        goto cleanup;
    }

    
    if (!lobby_add_client_reference(client)) {
        printf("Errore aggiunta client alla lobby\n");
        goto cleanup;
    }

    
    while (client->is_active && global_server->is_running) {
        int bytes = network_receive_from_client(client, buffer, sizeof(buffer));
        if (bytes <= 0) {
            break;
        }
        lobby_handle_client_message(client, buffer);
    }

cleanup:
    
    EnterCriticalSection(&lobby_mutex);
    if (strlen(client->name) > 0) {
        remove_name(client->name);
    }
    LeaveCriticalSection(&lobby_mutex);

    lobby_remove_client(client);
    closesocket(client->client_fd);
    free(client);
    return 0;
}

void flush_input_buffer() {
    while (_kbhit()) {
        _getch();
    }
}


DWORD WINAPI network_heartbeat_thread(LPVOID arg) {
    ServerNetwork *server = (ServerNetwork*)arg;
    
    while (server->is_running) {
        Sleep(10000); 
        
        
        lobby_broadcast_message("HEARTBEAT", NULL);
        
        
        lobby_check_timeouts();
        
        
        game_check_timeouts();
    }
    
    return 0;
}