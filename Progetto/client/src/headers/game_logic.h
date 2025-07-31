#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#define BOARD_SIZE 3

typedef enum {
    GAME_STATE_WAITING,
    GAME_STATE_PLAYING,
    GAME_STATE_OVER,  
    GAME_STATE_REMATCH
} GameState;

typedef enum {
    PLAYER_NONE = ' ',
    PLAYER_X = 'X',
    PLAYER_O = 'O'
} PlayerSymbol;

typedef struct {
    char board[BOARD_SIZE][BOARD_SIZE];
    PlayerSymbol current_player;
    GameState state;    
    PlayerSymbol winner;
    int is_draw;
} Game;

void game_init(Game *game);

int game_make_move(Game *game, int row, int col);
int game_is_valid_move(const Game *game, int row, int col);
void game_check_winner(Game *game);
void game_reset(Game *game);

void game_print_board(const Game *game);
int game_is_board_full(const Game *game);

int game_process_network_message(Game *game, const char *message);

void get_board_position(int move, int* row, int* col);

#endif 