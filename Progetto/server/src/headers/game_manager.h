#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include "network.h"

#define MAX_GAMES 50

typedef enum {
    GAME_STATE_WAITING,
    GAME_STATE_PLAYING,
    GAME_STATE_OVER
} GameState;

typedef enum {
    PLAYER_NONE = ' ',
    PLAYER_X = 'X',
    PLAYER_O = 'O'
} PlayerSymbol;

#ifdef _WIN32
#include <windows.h>
typedef CRITICAL_SECTION mutex_t;
#else
#include <pthread.h>
typedef pthread_mutex_t mutex_t;
#endif

typedef struct {
    int game_id;
    Client* player1;
    Client* player2;
    char board[3][3];
    PlayerSymbol current_player;
    GameState state;
    PlayerSymbol winner;
    int is_draw;
    mutex_t mutex;
} Game;

int game_manager_init();
void game_manager_cleanup();

int game_create_new(Client *creator);
int game_join(Client *client, int game_id);
void game_leave(Client *client);

int game_make_move(int game_id, Client *client, int row, int col);
void game_reset(int game_id);

Game* game_find_by_id(int game_id);
void game_list_available(char *response, size_t max_len);

void game_init_board(Game *game);
int game_check_winner(Game *game);
int game_is_valid_move(Game *game, int row, int col);
int game_is_board_full(Game *game);

#endif