#include "engine.h"
#include "io.h"
#include "mode.h"
#include "genmove.h"
#include "neighbours.h"
#include "liberties.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

void restore_board(GoBoard* Gb, Boards** boards) {
    Boards* current_head = *boards;
    memcpy(Gb->board, current_head->board, MAX_SIZE(Gb) * sizeof(*Gb->board));
    memcpy(Gb->chain_head, current_head->chain_head, MAX_SIZE(Gb) * sizeof(*Gb->chain_head));
    memcpy(Gb->next_stone, current_head->next_stone, MAX_SIZE(Gb) * sizeof(*Gb->next_stone));
    memcpy(Gb->chain_liberties, current_head->chain_liberties, MAX_SIZE(Gb) * sizeof(*Gb->chain_liberties));
    memcpy(Gb->chain_size, current_head->chain_size, MAX_SIZE(Gb) * sizeof(*Gb->chain_size));
    Gb->turn = current_head->turn;
    Gb->num_stones = current_head->num_stones;
    Gb->ko_pos = current_head->ko_pos;
    Gb->zobval = current_head->zobval;
}

void insert_board(GoBoard* Gb, Boards** boards) {
    Boards* current_head = *boards;
    Boards* new_head = malloc(sizeof(Boards));
    if (new_head == NULL) {
        print(NULL, STATUS_MEMORY_ALLOC_FAILED, "");
        exit(STATUS_FAILURE);
    }
    new_head->board = malloc(MAX_SIZE(Gb) * sizeof(*Gb->board));
    new_head->chain_head = malloc(MAX_SIZE(Gb) * sizeof(*Gb->chain_head));
    new_head->next_stone = malloc(MAX_SIZE(Gb) * sizeof(*Gb->next_stone));
    new_head->chain_liberties = malloc(MAX_SIZE(Gb) * sizeof(*Gb->chain_liberties));
    new_head->chain_size = malloc(MAX_SIZE(Gb) * sizeof(*Gb->chain_size));
    if (new_head->board == NULL || new_head->chain_head == NULL
        || new_head->next_stone == NULL || new_head->chain_liberties == NULL
        || new_head->chain_size == NULL) {
        print(NULL, STATUS_MEMORY_ALLOC_FAILED, "");
        exit(STATUS_FAILURE);
    }
    memcpy(new_head->board, Gb->board, MAX_SIZE(Gb) * sizeof(*Gb->board));
    memcpy(new_head->chain_head, Gb->chain_head, MAX_SIZE(Gb) * sizeof(*Gb->chain_head));
    memcpy(new_head->next_stone, Gb->next_stone, MAX_SIZE(Gb) * sizeof(*Gb->next_stone));
    memcpy(new_head->chain_liberties, Gb->chain_liberties, MAX_SIZE(Gb) * sizeof(*Gb->chain_liberties));
    memcpy(new_head->chain_size, Gb->chain_size, MAX_SIZE(Gb) * sizeof(*Gb->chain_size));
    new_head->turn = Gb->turn;
    new_head->num_stones = Gb->num_stones;
    new_head->ko_pos = Gb->ko_pos;
    new_head->zobval = Gb->zobval;
    new_head->pre_board = current_head;
    *boards = new_head;
}

void remove_last(Boards** boards) {
    Boards* temp;
    if (*boards != NULL) {
        temp = *boards;
        *boards = temp->pre_board;
        free(temp->board);
        free(temp->chain_head);
        free(temp->next_stone);
        free(temp->chain_liberties);
        free(temp->chain_size);
        free(temp);
    }
}

int32_t initialize_chain(GoBoard* Gb, int32_t position) {
    int32_t head = position;
    Gb->chain_head[head] = head;
    Gb->next_stone[head] = head;
    Gb->chain_size[head] = 1;
    return head;
}

int32_t initialize_chain_sim(GoBoard* Gb, int32_t position) {
    ChangeLog* log = Gb->log;
    int32_t head = position;
    set_int(log, &Gb->chain_head[position], position);
    set_int(log, &Gb->next_stone[position], position); 
    set_int(log, &Gb->chain_size[position], 1);
    return head;
}

// Generates a high-quality 64-bit random number (State must not be 0).
uint64_t xorshift64(uint64_t* state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return *state = x; // Updates the seed and returns the new random value.
}

int32_t init_zobrist_system(GoBoard* Gb) {
    if (Gb->zoboard == NULL) Gb->zoboard = malloc(2 * sizeof(uint64_t*));
    if (Gb->zoboard == NULL) {
        print(NULL, STATUS_MEMORY_ALLOC_FAILED, "");
        return STATUS_FAILURE;
    }
    if (Gb->zoboard[0] == NULL) Gb->zoboard[0] = malloc(MAX_SIZE(Gb) * sizeof(uint64_t));
    if (Gb->zoboard[1] == NULL) Gb->zoboard[1] = malloc(MAX_SIZE(Gb) * sizeof(uint64_t));
    if (Gb->zoboard[0] == NULL || Gb->zoboard[1] == NULL) {
        print(NULL, STATUS_MEMORY_ALLOC_FAILED, "");
        return STATUS_FAILURE;
    }
    // We use a constant seed so the "ID" of a board is the same every run.
    uint64_t seed = 88172645463325252ULL; 

    // Fill the tables.
    for (int32_t color_idx = 0; color_idx < 2; color_idx++) { // Last color is for xoring ko turns, not placements or captures.
        for (int32_t pos = 0; pos < MAX_SIZE(Gb); pos++) {
            Gb->zoboard[color_idx][pos] = xorshift64(&seed);
        }
    }

    // Initialize the special key for the turn flip.
    Gb->ZSideToMove = xorshift64(&seed);

    // Start the game hash at 0.
    Gb->zobval = 0;

    return STATUS_SUCCESS;
}

int main() {
    // Start the buffer with 32 characters. (passive read can realloc memory to the required ammount if needed)
    char* input_buffer = malloc(32 * sizeof(char));
    long msg_length;
    size_t msg_size = 32;

    Modes mode = MODE_UNKNOWN;
    Status status = STATUS_UNKNOWN;
    char* p = NULL;

    GoBoard* Gb = calloc(1, sizeof(GoBoard));
    GoBoard* sim = calloc(1, sizeof(GoBoard));
    TTEntry* TransTable = NULL;
    if (Gb == NULL || sim == NULL) {
        print(NULL, STATUS_MEMORY_ALLOC_FAILED, "");
        return STATUS_FAILURE;
    }
    Boards* boards = NULL;
    Gb->size = 21; // 21 (19 for the board) is the default.
    Gb->komi = 0.0;
    Gb->num_stones = 0;
    Gb->visited_counter = 1;
    Gb->ko_pos = -1;
    // Set default settings because they could be ignored and be disabled by default.
    Gb->settings.main_time = -1;
    Gb->settings.byo_yomi = 0;
    Gb->settings.byo_yomi_stones = 0;
    Gb->settings.color = EMPTY;
    Gb->settings.time = 100; // Set this to 100 so it can genmove if no time_left command is executed.
    Gb->settings.stones = 0;

    // Keep listening to the client.
    while ((msg_length = passive_read(&input_buffer, &msg_size, stdin)) != -1) {
        if ((p = (strstr(input_buffer, "known_command"))) != NULL) {
            p +=14;
            mode = MODE_KNOWN_COMMAND;
        } else if (strcmp(input_buffer, "quit") == 0) {
            mode = MODE_QUIT;
        } else if ((p = (strstr(input_buffer, "boardsize"))) != NULL) {
            p +=9;
            mode = MODE_BOARD_SIZE;
        } else if ((p = (strstr(input_buffer, "komi"))) != NULL) {
            p +=5;
            mode = MODE_KOMI;
        } else if (strcmp(input_buffer, "clear_board") == 0) {
            mode = MODE_CLEAR_BOARD;
        } else if (strcmp(input_buffer, "showboard") == 0) {
            mode = MODE_SHOW_BOARD;
        } else if ((p = (strstr(input_buffer, "genmove"))) != NULL) {
            p +=8;
            mode = MODE_GENMOVE;
        } else if ((p = (strstr(input_buffer, "play"))) != NULL) {
            p +=4;
            mode = MODE_PLAY_MOVE;
        } else if ((p = (strstr(input_buffer, "undo"))) != NULL) {
            mode = MODE_UNDO_MOVE;
        } else if ((p = (strstr(input_buffer, "list_commands"))) != NULL) {
            mode = MODE_LIST_COMMANDS;
        } else if ((p = (strstr(input_buffer, "protocol_version"))) != NULL) {
            mode = MODE_PROTOCOL_VERSION;
        } else if ((p = (strstr(input_buffer, "name"))) != NULL) {
            mode = MODE_NAME;
        } else if ((p = (strstr(input_buffer, "version"))) != NULL) {
            mode = MODE_VERSION;
        } else if ((p = (strstr(input_buffer, "final_score"))) != NULL) {
            mode = MODE_FINAL_SCORE;
        } else if ((p = (strstr(input_buffer, "time_settings"))) != NULL) {
            p +=14;
            mode = MODE_TIME_SETTINGS;
        } else if ((p = (strstr(input_buffer, "time_left"))) != NULL) {
            p +=10;
            mode = MODE_TIME_LEFT;
        } else if (strcmp(input_buffer, "") == 0) {
        } else {
            printf("unknown command\n");
        }

        status = use(mode, Gb, sim, &TransTable, &boards, p);
        if (status == STATUS_FAILURE) {
            use(MODE_QUIT, Gb, sim, &TransTable, &boards, p);
            return STATUS_FAILURE;
        } else if (status == STATUS_QUIT) break;
        mode = MODE_UNKNOWN;
    }

    return STATUS_SUCCESS;
}