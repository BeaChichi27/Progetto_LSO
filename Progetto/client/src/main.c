#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <conio.h>
#include "network.h"
#include "game_logic.h"
#include "ui.h"

void get_board_position(int move, int* row, int* col) {
    *row = (move - 1) / 3;
    *col = (move - 1) % 3;
}

void handle_create_game(NetworkConnection* conn) {
    if (!network_create_game(conn)) {
        ui_show_error(network_get_error());
        return;
    }

    ui_show_waiting_screen();
    time_t start_time = time(NULL);
    
    while (1) {
        if (difftime(time(NULL), start_time) > 30) {
            ui_show_message("Timeout: nessun avversario trovato");
            network_send(conn, "CANCEL", 0);
            return;
        }
        
        char message[MAX_MSG_SIZE];
        if (network_receive(conn, message, sizeof(message), 0) > 0) {
            if (strstr(message, "OPPONENT_JOINED")) {
                ui_show_message("Avversario trovato! La partita inizia...");
                return;
            }
        }
        
        if (_kbhit() && _getch() == 27) {
            network_send(conn, "CANCEL", 0);
            char response[MAX_MSG_SIZE];
            if (network_receive(conn, response, sizeof(response), 0) > 0) {
                if (strstr(response, "GAME_CANCELED")) {
                    ui_show_message("Partita cancellata");
                    return;
                }
            }
            return;
        }
        
        Sleep(100);
    }
}

void handle_join_game(NetworkConnection* conn) {
    ui_show_message("Richiesta lista partite...");
    network_send(conn, "LIST_GAMES", 0);
    
    char message[MAX_MSG_SIZE];
    if (network_receive(conn, message, sizeof(message), 0) <= 0) {
        ui_show_error(network_get_error());
        return;
    }
    
    ui_show_message(message);
    
    printf("Inserisci ID partita (0 per annullare): ");
    char input[10];
    fgets(input, sizeof(input), stdin);
    int game_id = atoi(input);
    
    if (game_id == 0) return;
    
    char join_msg[20];
    snprintf(join_msg, sizeof(join_msg), "JOIN:%d", game_id);
    network_send(conn, join_msg, 0);
    
    if (network_receive(conn, message, sizeof(message), 0) <= 0) {
        ui_show_error(network_get_error());
        return;
    }
    
    if (strstr(message, "JOIN_ACCEPTED")) {
        ui_show_message("Partita iniziata! Aspettando primo giocatore...");
    } else {
        ui_show_error("Impossibile unirsi alla partita");
    }
}

void game_loop(NetworkConnection* conn, Game* game, int is_host) {
    while (game->state != GAME_STATE_OVER) {
        ui_show_board(game->board);
        
        if ((is_host && game->current_player == PLAYER_X) || 
            (!is_host && game->current_player == PLAYER_O)) {
            int move = ui_get_player_move();
            if (move == 0) break;
            
            int row, col;
            get_board_position(move, &row, &col);
            
            if (game_make_move(game, row, col)) {
                char move_msg[20];
                snprintf(move_msg, sizeof(move_msg), "MOVE:%d,%d", row, col);
                network_send(conn, move_msg, 1);
            } else {
                ui_show_error("Mossa non valida");
            }
        } else {
            ui_show_message("Aspettando mossa avversario...");
            
            char message[MAX_MSG_SIZE];
            int bytes = network_receive(conn, message, sizeof(message), 1);
            
            if (bytes > 0 && strstr(message, "MOVE:")) {
                int row, col;
                sscanf(message, "MOVE:%d,%d", &row, &col);
                game_make_move(game, row, col);
            }
        }
    }
    
    ui_show_board(game->board);
    
    if (game->winner != PLAYER_NONE) {
        char msg[50];
        snprintf(msg, sizeof(msg), "%c ha vinto!", game->winner);
        ui_show_message(msg);
    } else if (game->is_draw) {
        ui_show_message("Pareggio!");
    }
}

int main() {
    if (!network_global_init()) {
        ui_show_error("Errore inizializzazione rete");
        return 1;
    }

    char player_name[50];
    if (!ui_get_player_name(player_name, sizeof(player_name))) {
        ui_show_error("Nome non valido");
        WSACleanup();
        return 1;
    }

    NetworkConnection conn;
    network_init(&conn);
    
    if (!network_connect_to_server(&conn)) {
        ui_show_error(network_get_error());
        WSACleanup();
        return 1;
    }
    
    if (!network_register_name(&conn, player_name)) {
        ui_show_error(network_get_error());
        network_disconnect(&conn);
        WSACleanup();
        return 1;
    }

    while (1) {
        int choice = ui_show_main_menu();
        
        switch (choice) {
            case 1:
                handle_create_game(&conn);
                {
                    Game game;
                    game_init(&game);
                    game_loop(&conn, &game, 1);
                    
                    if (ui_ask_rematch()) {
                        network_send(&conn, "REMATCH", 0);
                    }
                }
                break;
                
            case 2:
                handle_join_game(&conn);
                {
                    Game game;
                    game_init(&game);
                    game_loop(&conn, &game, 0);
                    
                    if (ui_ask_rematch()) {
                        network_send(&conn, "REMATCH", 0);
                    }
                }
                break;
                
            case 3:
                network_disconnect(&conn);
                WSACleanup();
                return 0;
        }
    }

    network_disconnect(&conn);
    WSACleanup();
    return 0;
}