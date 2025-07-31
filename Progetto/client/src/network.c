#include "headers/network.h"
#include "headers/ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static char last_error[256] = {0};

static void set_error(const char *msg) {
    strncpy(last_error, msg, sizeof(last_error) - 1);
    last_error[sizeof(last_error) - 1] = '\0';
}

int network_global_init() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        char error_msg[100];
        sprintf(error_msg, "WSAStartup fallito: %d", result);
        set_error(error_msg);
        return 0;
    }
    return 1;
}

int network_init(NetworkConnection *conn) {
    memset(conn, 0, sizeof(NetworkConnection));
    conn->tcp_sock = INVALID_SOCKET;
    conn->udp_sock = INVALID_SOCKET;
    return 1;
}

int network_connect_to_server(NetworkConnection *conn) {
    conn->tcp_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (conn->tcp_sock == INVALID_SOCKET) {
        set_error("Errore creazione socket TCP");
        return 0;
    }

    
    DWORD timeout = 5000;
    setsockopt(conn->tcp_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(conn->tcp_sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        set_error("Indirizzo server non valido");
        closesocket(conn->tcp_sock);
        conn->tcp_sock = INVALID_SOCKET;
        return 0;
    }

    if (connect(conn->tcp_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        set_error("Connessione al server fallita");
        closesocket(conn->tcp_sock);
        conn->tcp_sock = INVALID_SOCKET;
        return 0;
    }

    conn->udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (conn->udp_sock == INVALID_SOCKET) {
        set_error("Errore creazione socket UDP");
        closesocket(conn->tcp_sock);
        conn->tcp_sock = INVALID_SOCKET;
        return 0;
    }

    return 1;
}


int network_register_name(NetworkConnection *conn, const char *name) {
    if (!conn || conn->tcp_sock == INVALID_SOCKET || !name) {
        set_error("Connessione non valida");
        return 0;
    }

    char msg[MAX_MSG_SIZE];
    sprintf_s(msg, sizeof(msg), "REGISTER:%s", name);

    if (send(conn->tcp_sock, msg, (int)strlen(msg), 0) == SOCKET_ERROR) {
        set_error("Invio nome fallito");
        return 0;
    }

    char response[MAX_MSG_SIZE];
    int bytes = network_receive(conn, response, sizeof(response), 0);
    if (bytes <= 0) {
        set_error("Nessuna risposta dal server");
        return 0;
    }

    if (strstr(response, "REGISTRATION_OK") != NULL) {
        strncpy_s(conn->player_name, sizeof(conn->player_name), name, _TRUNCATE);
        return 1;
    } else if (strstr(response, "ERROR:")) {
        set_error(response + 6); 
        return 0;
    } else {
        set_error("Risposta non riconosciuta dal server");
        return 0;
    }
}

int network_create_game(NetworkConnection *conn) {
    const char *msg = "CREATE_GAME";
    if (send(conn->tcp_sock, msg, (int)strlen(msg), 0) == SOCKET_ERROR) {
        set_error("Invio richiesta creazione partita fallito");
        return 0;
    }
    return 1;
}

int network_join_game(NetworkConnection *conn, int game_id) {
    char msg[MAX_MSG_SIZE];
    sprintf_s(msg, sizeof(msg), "JOIN_GAME:%d", game_id);
    
    if (send(conn->tcp_sock, msg, (int)strlen(msg), 0) == SOCKET_ERROR) {
        set_error("Invio richiesta join partita fallito");
        return 0;
    }
    return 1;
}

int network_send_move(NetworkConnection *conn, int move) {
    if (conn->udp_sock == INVALID_SOCKET) {
        set_error("Connessione UDP non inizializzata");
        return 0;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    InetPton(AF_INET, TEXT(SERVER_IP), &server_addr.sin_addr);

    char msg[16];
    sprintf_s(msg, sizeof(msg), "MOVE:%d", move);

    if (sendto(conn->udp_sock, msg, (int)strlen(msg), 0, 
              (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        set_error("Invio mossa fallito");
        return 0;
    }
    return 1;
}


int network_receive(NetworkConnection *conn, char *buffer, size_t buf_size, int use_udp) {
    
    if (!conn || (use_udp == 0 && conn->tcp_sock == INVALID_SOCKET) || 
        (use_udp == 1 && conn->udp_sock == INVALID_SOCKET)) {
        set_error("Connessione non valida");
        return -1;
    }

    SOCKET sock = use_udp ? conn->udp_sock : conn->tcp_sock;
    
    
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    struct timeval timeout = {2, 0}; 
    
    int select_result = select(0, &readfds, NULL, NULL, &timeout);
    
    
    if (select_result == 0) {
        set_error("Timeout: server non raggiungibile");
        return 0;  
    }
    
    if (select_result == SOCKET_ERROR) {
        set_error("Errore nel controllo socket");
        return -1;
    }

    
    int bytes = recv(sock, buffer, (int)buf_size - 1, 0);
    
    if (bytes > 0) {
        buffer[bytes] = '\0';
        
        
        if (strcmp(buffer, "HEARTBEAT") == 0) {
            network_send(conn, "HEARTBEAT_ACK", use_udp);
            return 0;
        }
        
        
        if (strstr(buffer, "WAITING_OPPONENT")) {
            ui_show_waiting_screen();
        }
        
        return bytes;
    } 
    else if (bytes == 0) {
        set_error("Connessione chiusa dal server");
        return -1;
    } 
    else {
        
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT) {
            set_error("Timeout nella ricezione");
        } else {
            set_error("Errore nella ricezione");
        }
        return -1;
    }
}

int network_send(NetworkConnection *conn, const char *message, int use_udp) {
    if (!conn || !message) {
        return 0;
    }
    
    if (use_udp && conn->udp_sock != INVALID_SOCKET) {
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
        
        if (sendto(conn->udp_sock, message, (int)strlen(message), 0,
                  (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            return 0;
        }
    } else if (conn->tcp_sock != INVALID_SOCKET) {
        if (send(conn->tcp_sock, message, (int)strlen(message), 0) == SOCKET_ERROR) {
            return 0;
        }
    } else {
        return 0;
    }
    
    return 1;
}

void network_disconnect(NetworkConnection *conn) {
    if (conn->tcp_sock != INVALID_SOCKET) {
        closesocket(conn->tcp_sock);
        conn->tcp_sock = INVALID_SOCKET;
    }
    if (conn->udp_sock != INVALID_SOCKET) {
        closesocket(conn->udp_sock);
        conn->udp_sock = INVALID_SOCKET;
    }
}

const char *network_get_error() {
    return last_error;
}

void flush_input_buffer() {
    while (_kbhit()) {
        _getch();
    }
}



int network_receive_with_heartbeat(NetworkConnection *conn, char *buffer, size_t buf_size, int use_udp) {
    while (1) {
        int bytes = network_receive(conn, buffer, buf_size, use_udp);
        
        if (bytes <= 0) {
            return bytes; 
        }
        
        
        if (strcmp(buffer, "HEARTBEAT") == 0) {
            network_send(conn, "HEARTBEAT_ACK", use_udp);
            continue; 
        }
        
        return bytes; 
    }
}

int network_wait_for_message_with_cancel(NetworkConnection *conn, char *buffer, size_t buf_size, int timeout_sec) {
    time_t start_time = time(NULL);
    
    while (1) {
        
        if (difftime(time(NULL), start_time) > timeout_sec) {
            set_error("Timeout: nessuna risposta dal server");
            return 0;
        }
        
        
        if (_kbhit() && _getch() == 27) {
            set_error("Operazione annullata dall'utente");
            network_send(conn, "CANCEL", 0);
            return 0;
        }
        
        
        int bytes = network_receive_with_heartbeat(conn, buffer, buf_size, 0);
        if (bytes > 0) {
            return bytes;
        }
        if (bytes < 0) {
            return bytes; 
        }
        
        Sleep(100); 
    }
}

int network_connect_with_retry(NetworkConnection *conn, const char* player_name) {
    int reconnect_attempts = 0;
    const int max_reconnect_attempts = 3;
    
    while (reconnect_attempts < max_reconnect_attempts) {
        if (!network_connect_to_server(conn)) {
            reconnect_attempts++;
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), 
                    "Tentativo di connessione %d/%d fallito: %s", 
                    reconnect_attempts, max_reconnect_attempts, network_get_error());
            
            printf("ERRORE: %s\n", error_msg);
            
            if (reconnect_attempts < max_reconnect_attempts) {
                printf("Riprovo tra 3 secondi...\n");
                Sleep(3000);
                continue;
            } else {
                set_error("Impossibile connettersi al server dopo 3 tentativi");
                return 0;
            }
        }
        break; 
    }
    
    
    if (!network_register_name(conn, player_name)) {
        return 0;
    }
    
    return 1;
}