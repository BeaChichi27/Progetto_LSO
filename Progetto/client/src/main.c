#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>        
#include <winsock2.h>
#include <windows.h>
#include <conio.h>
#include "headers/network.h"
#include "headers/game_logic.h"
#include "headers/ui.h"

void get_board_position(int move, int* row, int* col) {
    *row = (move - 1) / 3;
    *col = (move - 1) % 3;
}

int handle_create_game(NetworkConnection* conn) {
    if (!network_create_game(conn)) {
        ui_show_error(network_get_error());
        return 0;
    }

    ui_show_waiting_screen();
    time_t start_time = time(NULL);
    
    while (1) {
        
        if (difftime(time(NULL), start_time) > 30) {
            ui_show_message("Timeout: nessun avversario trovato");
            network_send(conn, "CANCEL", 0);
            return 0;
        }
        
        char message[MAX_MSG_SIZE];
        int bytes = network_receive(conn, message, sizeof(message), 0);
        
        
        if (bytes < 0) {
            ui_show_error("Server disconnesso durante l'attesa");
            return -1; 
        }
        
        if (bytes > 0) {
            if (strstr(message, "OPPONENT_JOINED")) {
                ui_show_message("Avversario trovato! La partita inizia...");
                return 1; 
            }
            if (strstr(message, "ERROR:")) {
                ui_show_error(message);
                return 0;
            }
        }
        
        
        if (_kbhit() && _getch() == 27) {
            network_send(conn, "CANCEL", 0);
            char response[MAX_MSG_SIZE];
            if (network_receive(conn, response, sizeof(response), 0) > 0) {
                if (strstr(response, "GAME_CANCELED")) {
                    ui_show_message("Partita cancellata");
                    return 0;
                }
            }
            return 0;
        }
        
        Sleep(100);
    }
}

int handle_join_game(NetworkConnection* conn) {
    ui_show_message("Richiesta lista partite...");
    network_send(conn, "LIST_GAMES", 0);
    
    char message[MAX_MSG_SIZE];
    int bytes = network_receive(conn, message, sizeof(message), 0);
    
    if (bytes <= 0) {
        if (bytes < 0) {
            ui_show_error("Server disconnesso");
            return -1; 
        }
        ui_show_error(network_get_error());
        return 0;
    }
    
    ui_show_message(message);
    
    printf("Inserisci ID partita (0 per annullare): ");
    char input[10];
    fgets(input, sizeof(input), stdin);
    int game_id = atoi(input);
    
    if (game_id == 0) return 0;
    
    char join_msg[20];
    snprintf(join_msg, sizeof(join_msg), "JOIN:%d", game_id);
    network_send(conn, join_msg, 0);
    
    bytes = network_receive(conn, message, sizeof(message), 0);
    if (bytes <= 0) {
        if (bytes < 0) {
            ui_show_error("Server disconnesso");
            return -1;
        }
        ui_show_error(network_get_error());
        return 0;
    }
    
    if (strstr(message, "JOIN_ACCEPTED")) {
        ui_show_message("Partita iniziata! Aspettando primo giocatore...");
        return 1;
    } else {
        ui_show_error("Impossibile unirsi alla partita");
        return 0;
    }
}

int game_loop(NetworkConnection* conn, Game* game, int is_host) {
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
                
                if (!network_send(conn, move_msg, 1)) {
                    ui_show_error("Errore invio mossa - server disconnesso");
                    return -1; 
                }
            } else {
                ui_show_error("Mossa non valida");
            }
        } else {
            ui_show_message("Aspettando mossa avversario...");
            
            char message[MAX_MSG_SIZE];
            int bytes = network_receive(conn, message, sizeof(message), 1);
            
            
            if (bytes < 0) {
                ui_show_error("Server disconnesso durante la partita");
                return -1;
            }
            
            if (bytes > 0 && strstr(message, "MOVE:")) {
                int row, col;
                sscanf(message, "MOVE:%d,%d", &row, &col);
                game_make_move(game, row, col);
            }
            
            
            if (bytes > 0) {
                if (strstr(message, "OPPONENT_LEFT")) {
                    ui_show_message("L'avversario ha abbandonato la partita");
                    return 0;
                }
                if (strstr(message, "GAME_OVER")) {
                    game->state = GAME_STATE_OVER;
                    if (strstr(message, "WINNER:X")) {
                        game->winner = PLAYER_X;
                    } else if (strstr(message, "WINNER:O")) {
                        game->winner = PLAYER_O;
                    } else if (strstr(message, "DRAW")) {
                        game->is_draw = 1;
                    }
                }
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
    
    return 1; 
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
    
    
    int reconnect_attempts = 0;
    const int max_reconnect_attempts = 3;
    
    while (reconnect_attempts < max_reconnect_attempts) {
        if (!network_connect_to_server(&conn)) {
            reconnect_attempts++;
            char error_msg[100];
            snprintf(error_msg, sizeof(error_msg), 
                    "Tentativo di connessione %d/%d fallito: %s", 
                    reconnect_attempts, max_reconnect_attempts, network_get_error());
            ui_show_error(error_msg);
            
            if (reconnect_attempts < max_reconnect_attempts) {
                ui_show_message("Riprovo tra 3 secondi...");
                Sleep(3000);
                continue;
            } else {
                ui_show_error("Impossibile connettersi al server");
                WSACleanup();
                return 1;
            }
        }
        break; 
    }
    
    if (!network_register_name(&conn, player_name)) {
        ui_show_error(network_get_error());
        network_disconnect(&conn);
        WSACleanup();
        return 1;
    }

    
    while (1) {
        int choice = ui_show_main_menu();
        int result = 0;
        
        switch (choice) {
            case 1:
            {
                result = handle_create_game(&conn);
                if (result == -1) {
                    
                    ui_show_error("Connessione persa. Chiusura applicazione.");
                    network_disconnect(&conn);
                    WSACleanup();
                    return 1;
                }
                if (result == 1) {
                    
                    Game game;
                    game_init(&game);
                    game.state = GAME_STATE_PLAYING;
                    
                    int game_result = game_loop(&conn, &game, 1);
                    if (game_result == -1) {
                        ui_show_error("Connessione persa. Chiusura applicazione.");
                        network_disconnect(&conn);
                        WSACleanup();
                        return 1;
                    }
                    
                    if (game_result == 1 && ui_ask_rematch()) {
                        network_send(&conn, "REMATCH", 0);
                    }
                }
                break;
            }
            
            case 2:
            {
                result = handle_join_game(&conn);
                if (result == -1) {
                    ui_show_error("Connessione persa. Chiusura applicazione.");
                    network_disconnect(&conn);
                    WSACleanup();
                    return 1;
                }
                if (result == 1) {
                    Game game;
                    game_init(&game);
                    game.state = GAME_STATE_PLAYING;
                    
                    int game_result = game_loop(&conn, &game, 0);
                    if (game_result == -1) {
                        ui_show_error("Connessione persa. Chiusura applicazione.");
                        network_disconnect(&conn);
                        WSACleanup();
                        return 1;
                    }
                    
                    if (game_result == 1 && ui_ask_rematch()) {
                        network_send(&conn, "REMATCH", 0);
                    }
                }
                break;
            }
            
            case 3:
                network_disconnect(&conn);
                WSACleanup();
                return 0;
                
            default:
                ui_show_error("Scelta non valida");
                break;
        }
    }

    network_disconnect(&conn);
    WSACleanup();
    return 0;
}