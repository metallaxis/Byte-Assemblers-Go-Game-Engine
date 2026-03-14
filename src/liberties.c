#include "engine.h"
#include "liberties.h"
#include "neighbours.h"
#include "io.h"

#include <stdlib.h>

int32_t count_liberties(GoBoard* Gb, int32_t pos) {
    int32_t liberties = 0;

    int32_t dr[4] = {1, -1, 0, 0};
    int32_t dc[4] = {0, 0, 1, -1};

    for (int32_t i = 0; i < 4; i++) {
        int32_t dpos = pos + (Gb->size*dr[i]) + dc[i];
        StoneColors stone = Board(Gb, dpos);
        if (stone == EMPTY) liberties++;
    }
    return liberties;
}

int32_t count_chain_liberties(GoBoard* Gb, int32_t head_pos) {
    if (head_pos < 0 || Board(Gb, head_pos) == EMPTY) return 0;
    int32_t total_liberties = 0;

    RESET_VISITED_LOGIC(Gb);
    
    int32_t indx = head_pos;
    int32_t dr[4] = {1, -1, 0, 0};
    int32_t dc[4] = {0, 0, 1, -1};

    do {
        for (int32_t i = 0; i < 4; i++) {
            int32_t dpos = indx + Gb->size*dr[i] + dc[i];
            if (Board(Gb, dpos) == EMPTY && HAS_NOT_BEEN_VISITED(Gb, dpos)) {
                total_liberties++;
                MARK_VISITED(Gb, dpos);
            }
        }
        indx = Gb->next_stone[indx];
    } while(indx != head_pos);
    
    return total_liberties;
}

int32_t approx_chain_liberties(GoBoard* Gb, int32_t head_pos, int32_t num_max_liberties) {
    if (head_pos < 0 || Board(Gb, head_pos) == EMPTY) return 0;
    int32_t approx_liberties = 0;

    RESET_VISITED_LOGIC(Gb); 

    int32_t indx = head_pos;
    int32_t dr[4] = {1, -1, 0, 0};
    int32_t dc[4] = {0, 0, 1, -1};

    do {
        for (int32_t i = 0; i < 4; i++) {
            int32_t dpos = indx + Gb->size*dr[i] + dc[i];

            if (Board(Gb, dpos) == EMPTY && HAS_NOT_BEEN_VISITED(Gb, dpos)) {
                if (approx_liberties >= num_max_liberties) return approx_liberties;
                approx_liberties++;
                MARK_VISITED(Gb, dpos);
            }
        }
        if (approx_liberties >= num_max_liberties) break;
        indx = Gb->next_stone[indx];
    } while(indx != head_pos);
    
    return approx_liberties;
}