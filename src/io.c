#include "engine.h"
#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long passive_read(char **msgptr, size_t *n, FILE *stream) {
    if (msgptr == NULL || n == NULL) return -1;

    if (*msgptr == NULL) {
        *n = 128;
        *msgptr = malloc(*n);
        if (*msgptr == NULL) return -1;
    }

    size_t pos = 0;
    int32_t c;

    while ((c = fgetc(stream)) != EOF) {
        // If the string dosent fit, double the size.
        if (pos + 1 >= *n) {
            size_t new_size = *n * 2;
            char *new_ptr = realloc(*msgptr, new_size);
            if (new_ptr == NULL) return -1;
            *msgptr = new_ptr;
            *n = new_size;
        }
        if (c == '\n' || c == '#') {
            // Skip the info after comments.
            if (c == '#') {
                // Read and discard the rest of the line until a newline or EOF.
                while ((c = fgetc(stream)) != EOF && c != '\n');
            }
            break; // Stop reading for this line.
        }
        (*msgptr)[pos++] = (char)c; // Add the corresponding characters.
    }

    if (pos == 0 && c == EOF) return -1;

    (*msgptr)[pos] = '\0'; // Add null byte.
    return (long)pos;
}

void print(GoBoard* Gb, Status code, char* msg) {
    if (msg == NULL) msg = "";
    switch (code) {
        case STATUS_SUCCESS:
            printf("= %s\n\n", msg);
            break;
        case STATUS_FAILURE:
            fprintf(stderr, "? %s\n\n", msg);
            break;
        case STATUS_MEMORY_ALLOC_FAILED:
            fprintf(stderr, "Memory allocation failed\n");
            break;
        case STATUS_SHOW_BOARD:
            printf("=\n   ");
            for (int32_t col = 1; col < Gb->size-1; col++) {
                char label = 'A' + (col - 1);
                if (label >= 'I') label++; // Skip the letter 'I'.
                printf("%c ", label);
            }
            printf("\n");
            for (int32_t row = Gb->size-2; row > 0; row--) {
                printf("%2d ", row);
                for (int32_t col = 1; col < Gb->size-1; col++) {
                    StoneColors current_stone = BOARD(Gb, row, col);
                    char display_char = '.';
                    if (current_stone == BLACK) {
                        display_char = 'X';
                    } else if (current_stone == WHITE) {
                        display_char = 'O';
                    }
                    printf("%c ", display_char);
                }
                printf("%d\n", row);
            }
            printf("  ");
            for (int32_t col = 1; col < Gb->size-1; col++) {
                char label = 'A' + (col - 1);
                if (label >= 'I') label++; // Skip the letter 'I'.
                printf(" %c", label);
            }
            printf("\n");
            break;
        case STATUS_OUT_OF_BOUNDS:
            fprintf(stderr, "? illegal move: out of bounds\n\n");
            break;
        case STATUS_POSITION_OCCUPIED:
            fprintf(stderr, "? illegal move: occupied\n\n");
            break;
        case STATUS_SUICIDE_MOVE:
            fprintf(stderr, "? illegal move: suicide\n\n");
            break;
        case STATUS_KO_VIOLATION:
            fprintf(stderr, "? illegal move: ko\n\n");
            break;
        case STATUS_WRONG_TURN:
            fprintf(stderr, "? illegal move: wrong turn\n\n");
            break;
        default:
            printf("Error: Unknown error code %d.\n", code);
            break;
    }
    fflush(stdout); // For printing.
}