#include "headers/network.h"
#include "headers/lobby.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ServerNetwork *global_server = NULL;

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
    printf("Server in ascolto sulla porta %d\n", SERVER_PORT);
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
    printf("Nuova connessione da %s:%d\n", 
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
    int bytes_sent = send(client->client_fd, message, (int)strlen(message), 0);
    if (bytes_sent == SOCKET_ERROR) {
        printf("Errore invio messaggio: %d\n", WSAGetLastError());
        client->is_active = 0;
        return 0;
    }
    printf("Inviato a %s: %s\n", client->name, message);
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
    printf("Ricevuto da %s: %s\n", client->name, buffer);
    return bytes;
}

DWORD WINAPI network_handle_client_thread(LPVOID arg) {
    Client *client = (Client*)arg;
    char buffer[MAX_MSG_SIZE];
    while (client->is_active && global_server->is_running) {
        int bytes = network_receive_from_client(client, buffer, sizeof(buffer));
        if (bytes <= 0) {
            break;
        }
        lobby_handle_client_message(client, buffer);
    }
    lobby_remove_client(client);
    closesocket(client->client_fd);
    free(client);
    return 0;
}

DWORD WINAPI network_handle_udp_thread(LPVOID arg) {
    ServerNetwork *server = (ServerNetwork*)arg;
    char buffer[MAX_MSG_SIZE];
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    printf("Thread UDP avviato\n");
    while (server->is_running) {
        int bytes = recvfrom(server->udp_socket, buffer, sizeof(buffer) - 1, 0,
                           (struct sockaddr*)&client_addr, &addr_len);
        if (bytes <= 0) {
            int error = WSAGetLastError();
            if (error != WSAEINTR && server->is_running) {
                printf("Errore ricezione UDP: %d\n", error);
            }
            continue;
        }
        buffer[bytes] = '\0';
        printf("UDP ricevuto: %s da %s:%d\n", 
               buffer, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        if (strncmp(buffer, "MOVE:", 5) == 0) {
        }
    }
    printf("Thread UDP terminato\n");
    return 0;
}