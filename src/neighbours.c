#include "engine.h"
#include "neighbours.h"
#include "liberties.h"

#include <stdio.h>
#include <stdlib.h>

void find_neighbours(GoBoard* Gb, int32_t position, StoneColors* color,
    int32_t* num_friendlies, int32_t* position_friendlies, int32_t* num_enemies,
    int32_t* position_enemies) {

    int32_t dr[4] = {1, -1, 0, 0};
    int32_t dc[4] = {0, 0, 1, -1};

    // Identify neighbouring chains.
    for (int32_t i = 0; i < 4; i++) {
        int32_t dpos = position + Gb->size*dr[i] + dc[i];
        StoneColors dcolor = Board(Gb, dpos);
        if (dcolor == *color) {
            if (position_friendlies != NULL && num_friendlies != NULL) {
                position_friendlies[(*num_friendlies)++] = dpos;
            }
        } else if (dcolor != EMPTY && dcolor != BORDER && dcolor != *color) {
            if (position_enemies != NULL && num_enemies != NULL) {
                position_enemies[(*num_enemies)++] = dpos;
            }
        }
    }
}

int32_t merge_neighbours(GoBoard* Gb, int32_t pos_i, int32_t pos_j) {
    int32_t head_i = get_head(Gb, pos_i);
    int32_t head_j = get_head(Gb, pos_j);

    if (head_i == head_j) return head_i; // Already the same chain.

    int32_t small = head_i;
    int32_t large = head_j;
    if (Gb->chain_size[head_i] > Gb->chain_size[head_j]) {
        small = head_j;
        large = head_i;
    }

    Gb->chain_head[small] = large;
 
    // Update next pointers.
    int32_t tmp_head_big = Gb->next_stone[large];

    Gb->next_stone[large] = Gb->next_stone[small];
    Gb->next_stone[small] = tmp_head_big;

    int32_t curr = large;
    do {
        Gb->chain_head[curr] = large; // Flattening.
        curr = Gb->next_stone[curr];  // Move to the next stone in the circular list.
    } while (curr != large);          // Stop when we've looped back to the start.

    // Update the sizes.
    Gb->chain_size[large] += Gb->chain_size[small];
    Gb->chain_size[small] = 0;
    
    return large;
}

int32_t merge_neighbours_sim(GoBoard* Gb, int32_t pos_i, int32_t pos_j) {
    ChangeLog* log = Gb->log;
    int32_t head_i = get_head(Gb, pos_i);
    int32_t head_j = get_head(Gb, pos_j);

    if (head_i == head_j) return head_i; // Already the same chain.

    int32_t small = head_i;
    int32_t large = head_j;
    if (Gb->chain_size[head_i] > Gb->chain_size[head_j]) {
        small = head_j;
        large = head_i;
    }

    set_int(log, &Gb->chain_head[small], large);
 
    // Update next pointers.
    int32_t tmp_head_big = Gb->next_stone[large];

    set_int(log, &Gb->next_stone[large], Gb->next_stone[small]);
    set_int(log, &Gb->next_stone[small], tmp_head_big);
    
    // Update the sizes.
    set_int(log, &Gb->chain_size[large], Gb->chain_size[large] + Gb->chain_size[small]);
    set_int(log, &Gb->chain_size[small], 0);
    
    return large;
}