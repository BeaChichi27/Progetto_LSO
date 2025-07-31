#ifndef UI_H
#define UI_H

int ui_show_main_menu();

void ui_show_waiting_screen();

void ui_show_board(const char board[3][3]);

int ui_get_player_move();

void ui_show_message(const char *message);

void ui_show_error(const char *error);

int ui_ask_rematch();

void ui_clear_screen();

int ui_get_player_name(char *name, int max_length);

void ui_show_waiting_screen(void);

void ui_show_waiting_with_animation();
int ui_show_styled_menu();
int ui_show_post_game_menu();
int ui_ask_rematch_as_guest();
void ui_show_connection_status(int attempt, int max_attempts);

#endif