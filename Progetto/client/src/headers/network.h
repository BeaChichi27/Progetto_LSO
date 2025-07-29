#ifndef NETWORK_H
#define NETWORK_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define MAX_MSG_SIZE 1024
#define TIMEOUT_SEC 5

typedef struct {
    SOCKET tcp_sock;
    SOCKET udp_sock;
    int udp_port;
    char player_name[50];
} NetworkConnection;

int network_global_init();
int network_init(NetworkConnection *conn);
int network_connect_to_server(NetworkConnection *conn);
void network_disconnect(NetworkConnection *conn);

int network_register_name(NetworkConnection *conn, const char *name);
int network_create_game(NetworkConnection *conn);
int network_join_game(NetworkConnection *conn, int game_id);
int network_send_move(NetworkConnection *conn, int move);
int network_accept_rematch(NetworkConnection *conn);

int network_send(NetworkConnection *conn, const char *message, int use_udp);
int network_receive(NetworkConnection *conn, char *buffer, size_t buf_size, int use_udp);

const char *network_get_error();

#endif