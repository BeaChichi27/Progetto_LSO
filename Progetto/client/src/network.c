#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char last_error[256] = {0};

static void set_error(const char *msg) {
    strncpy(last_error, msg, sizeof(last_error) - 1);
    last_error[sizeof(last_error) - 1] = '\0';
}

int network_init(NetworkConnection *conn) {
    memset(conn, 0, sizeof(NetworkConnection));
    conn->tcp_sock = -1;
    conn->udp_sock = -1;
    return 1;
}

int network_connect_to_server(NetworkConnection *conn) {
    conn->tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (conn->tcp_sock < 0) {
        set_error("Errore creazione socket TCP");
        return 0;
    }

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    
    setsockopt(conn->tcp_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(conn->tcp_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(conn->tcp_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        set_error("Connessione al server fallita");
        close(conn->tcp_sock);
        conn->tcp_sock = -1;
        return 0;
    }

    conn->udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (conn->udp_sock < 0) {
        set_error("Errore creazione socket UDP");
        close(conn->tcp_sock);
        conn->tcp_sock = -1;
        return 0;
    }

    return 1;
}

int network_register_name(NetworkConnection *conn, const char *name) {
    if (!conn || conn->tcp_sock < 0 || !name) {
        set_error("Connessione non valida");
        return 0;
    }

    char msg[MAX_MSG_SIZE];
    snprintf(msg, sizeof(msg), "REGISTER:%s", name);

    if (send(conn->tcp_sock, msg, strlen(msg), 0) < 0) {
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
        strncpy(conn->player_name, name, sizeof(conn->player_name) - 1);
        return 1;
    } else {
        set_error(response);
        return 0;
    }
}

int network_create_game(NetworkConnection *conn) {
    const char *msg = "CREATE_GAME";
    if (send(conn->tcp_sock, msg, strlen(msg), 0) < 0) {
        set_error("Invio richiesta creazione partita fallito");
        return 0;
    }
    return 1;
}

int network_join_game(NetworkConnection *conn, int game_id) {
    char msg[MAX_MSG_SIZE];
    snprintf(msg, sizeof(msg), "JOIN_GAME:%d", game_id);
    
    if (send(conn->tcp_sock, msg, strlen(msg), 0) < 0) {
        set_error("Invio richiesta join partita fallito");
        return 0;
    }
    return 1;
}

int network_send_move(NetworkConnection *conn, int move) {
    if (conn->udp_sock < 0) {
        set_error("Connessione UDP non inizializzata");
        return 0;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    char msg[16];
    snprintf(msg, sizeof(msg), "MOVE:%d", move);

    if (sendto(conn->udp_sock, msg, strlen(msg), 0, 
              (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        set_error("Invio mossa fallito");
        return 0;
    }
    return 1;
}

int network_receive(NetworkConnection *conn, char *buffer, size_t buf_size, int use_udp) {
    if (!conn || (use_udp==0 && conn->tcp_sock < 0) || (use_udp==1 && conn->udp_sock < 0)) {
        set_error("Connessione non valida");
        return -1;
    }

    int sock = use_udp ? conn->udp_sock : conn->tcp_sock;
    int bytes = recv(sock, buffer, buf_size - 1, 0);
    
    if (bytes > 0) {
        buffer[bytes] = '\0';
    } else if (bytes == 0) {
        set_error("Connessione chiusa dal server");
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            set_error("Timeout nella ricezione");
        } else {
            set_error("Errore nella ricezione");
        }
    }
    
    return bytes;
}

void network_disconnect(NetworkConnection *conn) {
    if (conn->tcp_sock >= 0) {
        close(conn->tcp_sock);
        conn->tcp_sock = -1;
    }
    if (conn->udp_sock >= 0) {
        close(conn->udp_sock);
        conn->udp_sock = -1;
    }
}

const char *network_get_error() {
    return last_error;
}