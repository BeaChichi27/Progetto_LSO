#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <winsock2.h>
#include <windows.h>
#include "headers/network.h"
#include "headers/lobby.h"
#include "headers/game_manager.h"

static ServerNetwork server;
static int server_running = 1;

BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
        printf("\nRicevuto segnale di chiusura, spegnimento server...\n");
        server_running = 0;
        network_shutdown(&server);
        return TRUE;
    }
    return FALSE;
}

void setup_signal_handlers() {
    if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
        printf("Errore impostazione gestore segnali: %lu\n", GetLastError());
    }
}

int main() {
    printf("=== TRIS SERVER (Windows) ===\n");
    setup_signal_handlers();
    if (!network_init(&server)) {
        fprintf(stderr, "Errore inizializzazione rete\n");
        return 1;
    }
    if (!lobby_init()) {
        fprintf(stderr, "Errore inizializzazione lobby\n");
        network_shutdown(&server);
        return 1;
    }
    if (!game_manager_init()) {
        fprintf(stderr, "Errore inizializzazione game manager\n");
        lobby_cleanup();
        network_shutdown(&server);
        return 1;
    }
    if (!network_start_listening(&server)) {
        fprintf(stderr, "Errore avvio server\n");
        game_manager_cleanup();
        lobby_cleanup();
        network_shutdown(&server);
        return 1;
    }
    printf("Server avviato con successo!\n");
    printf("Premere Ctrl+C per fermare il server\n\n");
    while (server_running) {
        Client *new_client = network_accept_client(&server);
        if (!new_client) {
            if (server_running) {
                Sleep(100);
            }
            continue;
        }
        new_client->thread = CreateThread(NULL, 0, network_handle_client_thread, 
                                         new_client, 0, NULL);
        if (new_client->thread == NULL) {
            printf("Errore impostazione gestore segnali: %lu\n", GetLastError());
            closesocket(new_client->client_fd);
            free(new_client);
            continue;
        }
        CloseHandle(new_client->thread);
    }
    printf("Pulizia in corso...\n");
    game_manager_cleanup();
    lobby_cleanup();
    network_shutdown(&server);
    printf("Server spento correttamente\n");
    return 0;
}