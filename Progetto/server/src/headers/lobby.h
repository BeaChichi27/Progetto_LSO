#ifndef LOBBY_H
#define LOBBY_H

#include "network.h"
#include "game_manager.h"
#include <winsock2.h>
#include <windows.h>

#define MAX_CLIENTS 100

int lobby_init();
void lobby_cleanup();

Client* lobby_add_client(SOCKET client_fd, const char *name);
void lobby_remove_client(Client *client);
Client* lobby_find_client_by_fd(SOCKET fd);
Client* lobby_find_client_by_name(const char *name);

void lobby_handle_client_message(Client *client, const char *message);
void lobby_broadcast_message(const char *message, Client *exclude);

void lobby_handle_register(Client *client, const char *name);
void lobby_handle_create_game(Client *client);
void lobby_handle_join_game(Client *client, const char *message);
void lobby_handle_list_games(Client *client);
void lobby_handle_move(Client *client, const char *message);
void lobby_handle_rematch(Client *client);
CRITICAL_SECTION* lobby_get_mutex();
Client* lobby_get_client_by_index(int index);
int lobby_add_client_reference(Client *client);
void lobby_check_timeouts();
int is_name_duplicate(const char *name);
void add_name(const char *name);
void remove_name(const char *name);

#endif