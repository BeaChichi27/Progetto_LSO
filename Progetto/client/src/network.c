#include "headers/network.h"
<<<<<<< HEAD
#include "headers/ui.h"
=======
>>>>>>> ec896caf03b8621b7f4c6d06a56af8841981fd6e
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    DWORD timeout = TIMEOUT_SEC * 1000;
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
    int bytes = recv(conn->tcp_sock, response, sizeof(response) - 1, 0);
    if (bytes <= 0) {
        set_error("Nessuna risposta dal server");
        return 0;
    }
    response[bytes] = '\0';

    if (strstr(response, "OK") != NULL) {
        strncpy_s(conn->player_name, sizeof(conn->player_name), name, _TRUNCATE);
        return 1;
    } else {
        set_error(response);
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
    if (!conn || (use_udp==0 && conn->tcp_sock == INVALID_SOCKET) || 
        (use_udp==1 && conn->udp_sock == INVALID_SOCKET)) {
        set_error("Connessione non valida");
        return -1;
    }

    SOCKET sock = use_udp ? conn->udp_sock : conn->tcp_sock;
    int bytes = recv(sock, buffer, (int)buf_size - 1, 0);
    
    if (bytes > 0) {
        buffer[bytes] = '\0';
        if (strstr(buffer, "WAITING_OPPONENT")) {
            ui_show_waiting_screen();
        }
    } else if (bytes == 0) {
        set_error("Connessione chiusa dal server");
    } else {
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT) {
            set_error("Timeout nella ricezione");
        } else {
            set_error("Errore nella ricezione");
        }
    }
    
    return bytes;
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