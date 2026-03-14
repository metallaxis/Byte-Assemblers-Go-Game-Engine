#include "engine.h"
#include "mode.h"
#include "move.h"
#include "genmove.h"
#include "io.h"
#include "score.h"

#include <stdlib.h>
#include <string.h>

int32_t init_engine_memory(GoBoard* Gb, int32_t size) {
    if (Gb->board == NULL) Gb->board = malloc(size * size * sizeof(int32_t));
    if (Gb->board == NULL) {
            print(NULL, STATUS_MEMORY_ALLOC_FAILED, "");
            return STATUS_FAILURE;
        }
    if (Gb->chain_head == NULL) Gb->chain_head = malloc(size * size * sizeof(int32_t));
    if (Gb->next_stone == NULL) Gb->next_stone = malloc(size * size * sizeof(int32_t));
    if (Gb->chain_liberties == NULL) Gb->chain_liberties = malloc(size * size * sizeof(int32_t));
    if (Gb->chain_size == NULL) Gb->chain_size = malloc(size * size * sizeof(int32_t));
    if (Gb->chain_head == NULL || Gb->next_stone == NULL
        || Gb->chain_liberties == NULL || Gb->chain_size == NULL) {
        print(NULL, STATUS_MEMORY_ALLOC_FAILED, "");
        return STATUS_FAILURE;
    }
    if (Gb->visited_array == NULL) Gb->visited_array = malloc(size * size * sizeof(uint64_t));
    if (Gb->visited_array == NULL) {
        print(NULL, STATUS_MEMORY_ALLOC_FAILED, "");
        return STATUS_FAILURE;
    }
    return STATUS_SUCCESS;
}

int32_t init_star_points(GoBoard* Gb) {
    if (Gb->is_hoshi == NULL) Gb->is_hoshi = malloc(MAX_SIZE(Gb) * sizeof(int32_t));
    if (Gb->is_hoshi == NULL) {
        print(NULL, STATUS_MEMORY_ALLOC_FAILED, "");
        return STATUS_FAILURE;
    }

    int32_t lines[3];
    int32_t count = 0;

    if (Gb->size == 21) { // 19x19.
        lines[0] = 4; lines[1] = 10; lines[2] = 16; count = 3;
    } else if (Gb->size == 15) { // 13x13.
        lines[0] = 4; lines[1] = 7;  lines[2] = 10; count = 3;
    } else if (Gb->size == 11) { // 9x9.
        lines[0] = 3; lines[1] = 5;  lines[2] = 7;  count = 3;
    }

    // Clear the array.
    memset(Gb->is_hoshi, 0, MAX_SIZE(Gb) * sizeof(*Gb->is_hoshi));

    // Fill the 9 intersections.
    for (int32_t r = 0; r < count; r++) {
        for (int32_t c = 0; c < count; c++) {
            int32_t pos = POS(lines[r], lines[c], Gb->size);
            Gb->is_hoshi[pos] = 1;
        }
    }
    return STATUS_SUCCESS;
}

void free_engine_memory(GoBoard* Gb) {
    if (Gb != NULL) {
        if (Gb->board != NULL) {
            free(Gb->board);
            Gb->board = NULL;
        }
        if (Gb->chain_head != NULL) {
            free(Gb->chain_head);
            Gb->chain_head = NULL;
        }
        if (Gb->next_stone != NULL) {
            free(Gb->next_stone);
            Gb->next_stone = NULL;
        }
        if (Gb->chain_liberties != NULL) {
            free(Gb->chain_liberties);
            Gb->chain_liberties = NULL;
        }
        if (Gb->chain_size != NULL) {
            free(Gb->chain_size);
            Gb->chain_size = NULL;
        }
        if (Gb->visited_array != NULL) {
            free(Gb->visited_array);
            Gb->visited_array = NULL;
        }
    }
}

int32_t use(Modes mode, GoBoard* Gb, GoBoard* sim, TTEntry** TransTable, Boards** boards, char* p) {
    switch (mode) {
            case MODE_QUIT:
                free_engine_memory(Gb);
                if (Gb->is_hoshi != NULL) {
                    free(Gb->is_hoshi);
                    Gb->is_hoshi = NULL;
                }
                if (Gb->zoboard != NULL) {
                    if (Gb->zoboard[0] != NULL) {
                        free(Gb->zoboard[0]);
                    }
                    if (Gb->zoboard[1] != NULL) {
                        free(Gb->zoboard[1]);
                        Gb->zoboard[1] = NULL;
                    }
                    free(Gb->zoboard);
                    Gb->zoboard = NULL;
                }
                if (Gb != NULL) {
                    free(Gb);
                    Gb = NULL;
                }
                free_engine_memory(sim);
                if (sim != NULL) {
                    free(sim);
                    sim = NULL;
                }
                if (*TransTable != NULL) {
                    free(*TransTable);
                    *TransTable = NULL;
                }
                print(NULL, STATUS_SUCCESS, "");
                return STATUS_QUIT;
            case MODE_BOARD_SIZE:
                if (atoi(p) > 25 || atoi(p) < 5) {
                    print(NULL, STATUS_FAILURE, "unacceptable size");
                    return STATUS_FAILURE;
                }
                Gb->size = atoi(p) + 2; // +2 for borders.
                print(Gb, STATUS_SUCCESS, "");
                break;
            case MODE_CLEAR_BOARD:            
                if (init_engine_memory(Gb, Gb->size) != STATUS_SUCCESS) return STATUS_FAILURE;
                if (init_zobrist_system(Gb) != STATUS_SUCCESS) return STATUS_FAILURE;
                if (init_star_points(Gb) != STATUS_SUCCESS) return STATUS_FAILURE;
                if (init_engine_memory(sim, Gb->size) != STATUS_SUCCESS) return STATUS_FAILURE;

                // Clear Transposition Table & Setup Zoboard.
                if (*TransTable == NULL) {
                    void* ptr;
                    if (posix_memalign(&ptr, 64, TT_SIZE * sizeof(TTEntry)) != 0) {
                        print(NULL, STATUS_MEMORY_ALLOC_FAILED, "");
                        exit(STATUS_FAILURE);
                    }
                    *TransTable = (TTEntry*)ptr;
                }
                memset(*TransTable, 0, TT_SIZE * sizeof(TTEntry));
                sim->zoboard = Gb->zoboard;
                sim->ZSideToMove = Gb->ZSideToMove;
                sim->is_hoshi = Gb->is_hoshi;

                for (int32_t i = 0; i < MAX_SIZE(Gb); i++) {
                    Board(Gb, i) = EMPTY; // Set the board to EMPTY (0).
                    // Set the default values for all the chain info.
                    Gb->chain_head[i] = -1;
                    Gb->next_stone[i] = -1;
                    Gb->chain_liberties[i] = 0;
                    Gb->chain_size[i] = 0;
                    Gb->visited_array[i] = 0;
                }

                for (int32_t i = 0; i < Gb->size; i++) {
                    // Set the borders.
                    BOARD(Gb, 0, i) = BORDER; BOARD(Gb, Gb->size-1, i) = BORDER;
                    BOARD(Gb, i, 0) = BORDER; BOARD(Gb, i, Gb->size-1) = BORDER;
                }
                insert_board(Gb, boards);

                Gb->turn = TURN_BLACK;
                print(Gb, STATUS_SUCCESS, "");
                break;
            case MODE_SHOW_BOARD:
                print(Gb, STATUS_SHOW_BOARD, "");
                break;
            case MODE_KOMI:
                Gb->komi = atof(p);
                print(Gb, STATUS_SUCCESS, "");
                break;
            case MODE_GENMOVE: {
                while(*p == ' ') p++; // Skip spaces.
                StoneColors color;
                if (p[0] == 'b' || p[0] == 'B') color = BLACK;
                else if (p[0] == 'w' || p[0] == 'W') color = WHITE;
                else {
                    print(Gb, STATUS_FAILURE, "invalid color");
                    break;
                }

                Turn played = color == WHITE ? TURN_WHITE : TURN_BLACK;
                if (Gb->turn != played) {
                    print(Gb, STATUS_WRONG_TURN, "");
                    break;
                }

                int32_t position = genmove(Gb, sim, color, *TransTable);
                if (position == -1) {
                    print(Gb, STATUS_SUCCESS, "PASS");
                    // Save this board to the previous.
                    insert_board(Gb, boards);
                    break;
                } else {
                    int32_t row = position / Gb->size; // Internal row.
                    int32_t col = (position % Gb->size) -1; // Internal col.

                    char letter = 'A' + col;
                    if (letter >= 'I') letter++; // Skip the letter 'I'.
                    printf("= %c%d\n\n", letter, row);
                    // Save this board to the previous.
                    insert_board(Gb, boards);
                    break;
                }
            }
            case MODE_PLAY_MOVE: {
                while(*p == ' ') p++; // Skip spaces.
                StoneColors color;
                if (p[0] == 'b' || p[0] == 'B') color = BLACK;
                else if (p[0] == 'w' || p[0] == 'W') color = WHITE;
                else {
                    print(Gb, STATUS_FAILURE, "invalid color");
                    break;
                }
                // Move past the color.
                if (strncasecmp(p, "white", 5) == 0) p += 5;
                else if (strncasecmp(p, "black", 5) == 0) p += 5;
                else p += 1;
                while(*p == ' ') p++; // Skip spaces.

                char col_char = p[0];
                if (col_char >= 'a' && col_char <= 'z') col_char -= 32; // Convert to uppercase.
                int32_t col = col_char - 'A' + 1;
                if (col_char > 'I') col--; // Skip the letter 'I'.
                int32_t row = atoi(p + 1); // Convert number to row index.
                int32_t position;

                if (strstr(p, "PASS") != NULL || strstr(p, "pass") != NULL) position = -1;
                else position = POS(row, col, Gb->size);

                Status code;
                // Check for out of bounds.
                if ((position != -1) && (col > Gb->size - 2 || row > Gb->size - 2 || col < 1 || row < 1)) {
                    code = STATUS_OUT_OF_BOUNDS;
                } else code = put_move(Gb, position, color);
                print(Gb, code, "");
                // Save this board to the previous.
                if (code == STATUS_SUCCESS) insert_board(Gb, boards);
                break;
            }
            case MODE_UNDO_MOVE:
                if (boards == NULL || (*boards)->pre_board == NULL) {
                    print(Gb, STATUS_FAILURE, "cannot undo");
                } else {
                    undo_previous_move(Gb, boards);
                    print(Gb, STATUS_SUCCESS, "");
                }
                break;
            case MODE_LIST_COMMANDS:
                printf("protocol_version       Ask what GTP version the engine supports\n");
                printf("name                   Engine name\n");
                printf("version                Engine version string\n");
                printf("list_commands          List all supported commands\n");
                printf("known_command <cmd>    Check whether the specificied command exists\n");
                printf("boardsize N            Set board size (e.g., boardsize 19)\n");
                printf("clear_board            Clear the board (start a new game)\n");
                printf("komi x.x               Set komi (points compensation)\n");
                printf("play <color> <vertex>  Play a move (e.g., play B D4)\n");
                printf("genmove <color>        Ask engine to generate a move\n");
                printf("showboard              Print or report the current board state\n");
                printf("final_score            Return the final score when the game has ended\n");
                printf("time_settings M B S    Set main time M, byo-yomi time B, and stones S\n");
                printf("time_left C T S        Update remaining time T and byo-yomi stones S for color C\n");
                printf("undo                   Undo last move\n");
                printf("quit                   Terminate the engine\n");
                break;
            case MODE_PROTOCOL_VERSION:
                print(Gb, STATUS_SUCCESS, "2.0");
                break;
            case MODE_NAME:
                print(Gb, STATUS_SUCCESS, "BA Go");
                break;
            case MODE_VERSION:
                print(Gb, STATUS_SUCCESS, "3.5");
                break;
            case MODE_FINAL_SCORE: {
                float final_score = calculate_score(Gb);
                if (final_score > 0) printf("= %c+%.1f\n\n", 'B', final_score);
                else if (final_score < 0) printf("= %c+%.1f\n\n", 'W', -final_score);
                else print(Gb, STATUS_SUCCESS, "0");
                break;
            }
            case MODE_TIME_SETTINGS:
                if (sscanf(p, "%d %d %d", &Gb->settings.main_time, 
                    &Gb->settings.byo_yomi, 
                    &Gb->settings.byo_yomi_stones) == 3) {
                    print(Gb, STATUS_SUCCESS, "");
                } else {
                    print(Gb, STATUS_FAILURE, "invalid time settings arguments");
                }
                break;
            case MODE_TIME_LEFT: {
                char temp;
                while(*p == ' ') p++; // Skip spaces.
                if (sscanf(p, "%c", &temp) == 1 && (temp == 'B' || temp == 'W')) {
                    Gb->settings.color = temp == 'B' ? BLACK : WHITE;
                } else {
                    print(Gb, STATUS_FAILURE, "invalid time left arguments");
                }
                p++; // Skip the color.
                while(*p == ' ') p++; // Skip spaces.
                if (sscanf(p, "%d %d", &Gb->settings.time, &Gb->settings.stones) == 2) {
                    print(Gb, STATUS_SUCCESS, "");
                } else {
                    print(Gb, STATUS_FAILURE, "invalid time left arguments");
                }
                break;
            }
            case MODE_KNOWN_COMMAND:
                while(*p == ' ') p++; // Skip spaces.
                if ((strcmp(p, "protocol_version")) == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else if ((strcmp(p, "name")) == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else if (strcmp(p, "version") == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else if (strcmp(p, "list_commands") == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else if ((strcmp(p, "known_command")) == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else if ((strcmp(p, "boardsize")) == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else if (strcmp(p, "clear_board") == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else if (strcmp(p, "komi") == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else if ((strcmp(p, "play")) == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else if ((strcmp(p, "genmove")) == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else if (strcmp(p, "showboard") == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else if (strcmp(p, "final_score") == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else if (strcmp(p, "time_settings") == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else if (strcmp(p, "time_left") == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else if (strcmp(p, "undo") == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else if (strcmp(p, "quit") == 0) {
                    print(Gb, STATUS_SUCCESS, "true");
                } else {
                    print(Gb, STATUS_SUCCESS, "false");
                }
                break;
            default:
                break;
        }
        fflush(stdout); // For printing.
    return STATUS_SUCCESS;
}