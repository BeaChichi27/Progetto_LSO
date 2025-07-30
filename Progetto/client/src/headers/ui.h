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

<<<<<<< HEAD
void ui_show_waiting_screen(void);

=======
>>>>>>> ec896caf03b8621b7f4c6d06a56af8841981fd6e
#endif