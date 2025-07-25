#include "game_logic.h"
#include <stdio.h>
#include <string.h>

void game_init(Game *game) {
    memset(game, 0, sizeof(Game));
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            game->board[i][j] = PLAYER_NONE;
        }
    }
    
    game->current_player = PLAYER_X;
    game->state = GAME_STATE_WAITING;
    game->winner = PLAYER_NONE;
    game->is_draw = 0;
}

int game_make_move(Game *game, int row, int col) {
    if (!game_is_valid_move(game, row, col)) {
        return 0;
    }
    
    game->board[row][col] = (char)game->current_player;
    
    game_check_winner(game);
    
    if (game->state == GAME_STATE_PLAYING) {
        game->current_player = (game->current_player == PLAYER_X) ? PLAYER_O : PLAYER_X;
    }
    
    return 1;
}

int game_is_valid_move(const Game *game, int row, int col) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return 0;
    }
    
    return (game->board[row][col] == PLAYER_NONE);
}

void game_check_winner(Game *game) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (game->board[i][0] != PLAYER_NONE &&
            game->board[i][0] == game->board[i][1] && 
            game->board[i][1] == game->board[i][2]) {
            game->winner = (PlayerSymbol)game->board[i][0];
            game->state = GAME_STATE_OVER;
            return;
        }
    }
    
    for (int j = 0; j < BOARD_SIZE; j++) {
        if (game->board[0][j] != PLAYER_NONE &&
            game->board[0][j] == game->board[1][j] && 
            game->board[1][j] == game->board[2][j]) {
            game->winner = (PlayerSymbol)game->board[0][j];
            game->state = GAME_STATE_OVER;
            return;
        }
    }
    
    if (game->board[0][0] != PLAYER_NONE &&
        game->board[0][0] == game->board[1][1] && 
        game->board[1][1] == game->board[2][2]) {
        game->winner = (PlayerSymbol)game->board[0][0];
        game->state = GAME_STATE_OVER;
        return;
    }
    
    if (game->board[0][2] != PLAYER_NONE &&
        game->board[0][2] == game->board[1][1] && 
        game->board[1][1] == game->board[2][0]) {
        game->winner = (PlayerSymbol)game->board[0][2];
        game->state = GAME_STATE_OVER;
        return;
    }
    
    if (game_is_board_full(game)) {
        game->is_draw = 1;
        game->state = GAME_STATE_OVER;
    }
}

void game_reset(Game *game) {
    game_init(game);
    game->state = GAME_STATE_PLAYING;
}

void game_print_board(const Game *game) {
    printf("\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            printf(" %c ", game->board[i][j]);
            if (j < BOARD_SIZE - 1) printf("|");
        }
        printf("\n");
        
        if (i < BOARD_SIZE - 1) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                printf("---");
                if (j < BOARD_SIZE - 1) printf("+");
            }
            printf("\n");
        }
    }
    printf("\n");
}

int game_is_board_full(const Game *game) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (game->board[i][j] == PLAYER_NONE) {
                return 0;
            }
        }
    }
    return 1;
}

int game_process_network_message(Game *game, const char *message) {
    if (strncmp(message, "MOVE:", 5) == 0) {
        // Formato: "MOVE:<row>,<col>"
        int row, col;
        if (sscanf(message + 5, "%d,%d", &row, &col) == 2) {
            return game_make_move(game, row, col);
        }
    }
    else if (strcmp(message, "RESET") == 0) {
        game_reset(game);
        return 1;
    }
    else if (strcmp(message, "REMATCH") == 0) {
        game->state = GAME_STATE_REMATCH;
        return 1;
    }
    
    return 0;
}