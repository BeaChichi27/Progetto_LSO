#include "headers/ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h> 
#include <windows.h> 

void ui_clear_screen() {
    system("cls");
}

int ui_show_main_menu() {
    ui_clear_screen();
    printf("\n");
    printf("=== TRIS ONLINE ===\n");
    printf("1. Crea nuova partita\n");
    printf("2. Unisciti a partita\n");
    printf("3. Esci\n");
    printf("\nScelta: ");

    int choice = 0;
    while (1) {
        char input = _getch();
        if (input >= '1' && input <= '3') {
            choice = input - '0';
            printf("%d\n", choice);
            break;
        }
    }
    
    return choice;
}

<<<<<<< HEAD
=======
void ui_show_waiting_screen() {
    ui_clear_screen();
    printf("\nIn attesa di un avversario... (Timeout: 5 minuti)\n");
    printf("Premi ESC per annullare\n");
}

>>>>>>> ec896caf03b8621b7f4c6d06a56af8841981fd6e
void ui_show_board(const char board[3][3]) {
    ui_clear_screen();
    printf("\n");
    printf(" %c | %c | %c \n", board[0][0], board[0][1], board[0][2]);
    printf("---+---+---\n");
    printf(" %c | %c | %c \n", board[1][0], board[1][1], board[1][2]);
    printf("---+---+---\n");
    printf(" %c | %c | %c \n", board[2][0], board[2][1], board[2][2]);
    printf("\n");
}

int ui_get_player_move() {
    printf("Scegli una cella (1-9) o 0 per uscire: ");
    
    while (1) {
        char input = _getch();
        if (input == '0') {
            printf("0\n");
            return 0;
        }
        if (input >= '1' && input <= '9') {
            printf("%c\n", input);
            return input - '0';
        }
    }
}

void ui_show_message(const char *message) {
    printf("\n%s\n", message);
    printf("Premi un tasto per continuare...");
    _getch();
}

void ui_show_error(const char *error) {
    printf("\nERRORE: %s\n", error);
    printf("Premi un tasto per continuare...");
    _getch();
}

int ui_ask_rematch() {
    printf("\nVuoi fare una rivincita? (s/n): ");
    
    while (1) {
        char input = _getch();
        if (input == 's' || input == 'S') {
            printf("s\n");
            return 1;
        }
        if (input == 'n' || input == 'N') {
            printf("n\n");
            return 0;
        }
    }
}

int ui_get_player_name(char *name, int max_length) {
    ui_clear_screen();
    printf("\nInserisci il tuo nome (max %d caratteri): ", max_length - 1);
    
    if (fgets(name, max_length, stdin) == NULL) {
        return 0;
    }
    
    name[strcspn(name, "\n")] = '\0';
    
    if (strlen(name) == 0) {
        return 0;
    }
    
    return 1;
<<<<<<< HEAD
}

void ui_show_waiting_screen(void) {
    ui_clear_screen();
    
    printf("\n");
    printf("====================================\n");
    printf("        IN ATTESA DI RISPOSTA       \n");
    printf("====================================\n");
    printf("\n");
    printf("   [.  ]  Connessione in corso...   \n");
    printf("\n");
    printf("   Premere Ctrl+C per annullare     \n");
    printf("\n");
    printf("====================================\n");
    
    fflush(stdout);
=======
>>>>>>> ec896caf03b8621b7f4c6d06a56af8841981fd6e
}