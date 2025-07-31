#ifndef SERVER_NETWORK_H
#define SERVER_NETWORK_H

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <conio.h>

#define SERVER_PORT 8080
#define MAX_MSG_SIZE 1024
#define MAX_NAME_LEN 50

typedef struct {
    SOCKET client_fd;
    char name[MAX_NAME_LEN];
    int game_id;
    char symbol;
    struct sockaddr_in udp_addr;
    int is_active;
    HANDLE thread;
    time_t last_heartbeat_ack;
} Client;

typedef struct {
    SOCKET tcp_socket;
    SOCKET udp_socket;
    struct sockaddr_in server_addr;
    int is_running;
} ServerNetwork;

int network_init(ServerNetwork *server);
int network_start_listening(ServerNetwork *server);
void network_shutdown(ServerNetwork *server);

Client* network_accept_client(ServerNetwork *server);
int network_send_to_client(Client *client, const char *message);
int network_receive_from_client(Client *client, char *buffer, size_t buf_size);

DWORD WINAPI network_handle_client_thread(LPVOID arg);
DWORD WINAPI network_handle_udp_thread(LPVOID arg);
DWORD WINAPI network_heartbeat_thread(LPVOID arg);

void flush_input_buffer();

#endif