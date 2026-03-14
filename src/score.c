#include "engine.h"
#include "score.h"

// Helper to check ownership of an empty region.
// Returns: BLACK, WHITE, or EMPTY (if neutral/dame).
// Output: *points = number of empty spots in this cluster.
StoneColors get_territory_owner(GoBoard* Gb, int32_t start_pos, int32_t* points) {
    int32_t touches_black = 0;
    int32_t touches_white = 0;
    *points = 0;

    int32_t stack[1024];
    int32_t top = 0;

    // Push start
    stack[top++] = start_pos;
    MARK_VISITED(Gb, start_pos);

    // Directions for neighbors (Left, Right, Down, Up).
    int32_t delta[4] = {-1, 1, Gb->size, -Gb->size};

    while (top > 0) {
        int32_t curr = stack[--top];
        (*points)++;
        for (int32_t i = 0; i < 4; i++) {
            int32_t neighbor = curr + delta[i];
            StoneColors c = Gb->board[neighbor];

            if (c == BORDER) continue;

            if (c == BLACK) {
                touches_black = 1;
            } else if (c == WHITE) {
                touches_white = 1;
            } else if (c == EMPTY) {
                // If we haven't visited this empty spot yet, add to stack.
                if (HAS_NOT_BEEN_VISITED(Gb, neighbor)) {
                    MARK_VISITED(Gb, neighbor);
                    stack[top++] = neighbor;
                }
            }
        }
    }

    if (touches_black && !touches_white) return BLACK;
    if (touches_white && !touches_black) return WHITE;
    return EMPTY; // Neutral.
}

// Calculates final score: Black Score - (White Score + Komi).
float calculate_score(GoBoard* Gb) {
    float black_score = 0.0;
    float white_score = 0.0;
    int32_t N = Gb->size * Gb->size;

    // 1. New "Session" for the visited array (O(1) clear).
    RESET_VISITED_LOGIC(Gb); 

    // 2. Iterate over the entire board array.
    for (int32_t i = Gb->size + 1; i < N - Gb->size; i++) {
        StoneColors c = Gb->board[i];

        if (c == BORDER) continue;

        if (c == BLACK) {
            black_score += 1.0;
        } else if (c == WHITE) {
            white_score += 1.0;
        } else if (c == EMPTY) {
            // If this empty spot hasn't been counted in a territory yet.
            if (Gb->visited_array[i] != Gb->visited_counter) {
                int32_t area_points = 0;
                StoneColors owner = get_territory_owner(Gb, i, &area_points);
                
                if (owner == BLACK) {
                    black_score += (float)area_points;
                } else if (owner == WHITE) {
                    white_score += (float)area_points;
                }
            }
        }
    }

    // Return the difference (Black - White) adjusted for Komi.
    return black_score - (white_score + Gb->komi);
}