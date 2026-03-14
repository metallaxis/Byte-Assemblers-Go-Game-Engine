#include "engine.h"
#include "move.h"
#include "io.h"
#include "neighbours.h"
#include "liberties.h"

#include <stdlib.h>
#include <string.h>

void undo_previous_move(GoBoard* Gb, Boards** boards) {
    Gb->turn = Gb->turn == TURN_BLACK ? TURN_WHITE : TURN_BLACK;
    remove_last(boards);
    restore_board(Gb, boards);
}

int32_t put_move(GoBoard* Gb, int32_t position, StoneColors color) {
    Turn played = color == WHITE ? TURN_WHITE : TURN_BLACK;
    if (Gb->turn != played) return STATUS_WRONG_TURN;

    // Check for PASS.
    if (position == -1) {
        Gb->ko_pos = -1; // Ko is always cleared on a pass.
        Gb->turn = color == BLACK ? TURN_WHITE : TURN_BLACK;
        Gb->zobval ^= Gb->ZSideToMove;                           
        return STATUS_SUCCESS;
    }
    
    // Check if position is occupied.
    if (Board(Gb, position) != EMPTY) return STATUS_POSITION_OCCUPIED;

    // Ko violation check.
    if (Gb->ko_pos == position) return STATUS_KO_VIOLATION;

    // Find neighbouring chains.
    int32_t num_friendlies = 0;
    int32_t position_friendlies[4] = {-1, -1, -1, -1};
    int32_t num_enemies = 0;
    int32_t position_enemies[4] = {-1, -1, -1, -1};
    int32_t friend_heads[4] = {-1, -1, -1, -1};
    int32_t enemy_heads[4] = {-1, -1, -1, -1};
    find_neighbours(Gb, position, &color, &num_friendlies, position_friendlies, &num_enemies, position_enemies);

    // Suicide logic.
    unsigned char conditions[2] = {1, 0};
    // Condition: if all neighbour chains have 1 liberty left.
    for (int32_t i = 0; i < num_friendlies; i++) {
        friend_heads[i] = get_head(Gb, position_friendlies[i]);
        if (position_friendlies[i] >= 0) {
            conditions[0] &= (Gb->chain_liberties[friend_heads[i]] == 1);
        }
    }
    
    // Condition: if at least one enemy chain has 1 liberty left.
    for (int32_t i = 0; i < num_enemies; i++) {
        enemy_heads[i] = get_head(Gb, position_enemies[i]);
        if (position_enemies[i] >= 0) {
            conditions[1] |= (Gb->chain_liberties[enemy_heads[i]] == 1);
        }
    }

    // Suicide check.
    if (conditions[0] && count_liberties(Gb, position) == 0 && !conditions[1]) {
        return STATUS_SUICIDE_MOVE;
    }

    // Place the stone.
    Board(Gb, position) = color;

    // Merge all friendly neighbours into a single unified chain.
    int32_t head = -1;
    if (num_friendlies > 0) {
        head = friend_heads[0];
        for (int32_t i = 1; i < num_friendlies; i++) {
            head = merge_neighbours(Gb, head, friend_heads[i]);
        }
    }
    // If there where no friendly neighbours, make a new chain.
    if (head == -1) {
        head = initialize_chain(Gb, position);
    } else { // Else enclude the linking stone. (the one that links the num_friendlies chains)
        int32_t tmp_next_head = Gb->next_stone[head];
        Gb->next_stone[head] = position;
        Gb->next_stone[position] = tmp_next_head;
        Gb->chain_head[position] = head;
        Gb->chain_size[head]++;
    }
    // And count the liberties of the merged chain.
    Gb->chain_liberties[head] = count_chain_liberties(Gb, head);

    Gb->zobval ^= Gb->zoboard[color-2][position];

    // --- Enemy capturing logic ---
    RESET_VISITED_LOGIC(Gb);
    for (int32_t i = 0; i < num_enemies; i++) {
       if (HAS_NOT_BEEN_VISITED(Gb, enemy_heads[i])) {
            MARK_VISITED(Gb, enemy_heads[i]);
            Gb->chain_liberties[enemy_heads[i]]--;
       }
    }
    int32_t num_captured = 0;
    int32_t last_captured_pos = -1;
    for (int32_t i = 0; i < num_enemies; i++) {
        int32_t ecolor = (color == WHITE) ? BLACK : WHITE;
        int32_t enemy_head = enemy_heads[i];

        // If the chain has already been freed, continue the loop.
        if (enemy_head == -1) continue;
        if (Board(Gb, enemy_head) == EMPTY) continue;
    
        if (Gb->chain_liberties[enemy_head] == 0) {
            int32_t indx = enemy_head;
            do {
                int32_t next = Gb->next_stone[indx];
                if (Board(Gb, indx) != EMPTY) {
                    Gb->zobval ^= Gb->zoboard[ecolor-2][indx];
                    // Remove enemy.
                    Board(Gb, indx) = EMPTY;
                    Gb->num_stones--;
                }
                Gb->chain_head[indx] = -1;
                int32_t dr[4] = {1, -1, 0, 0};
                int32_t dc[4] = {0, 0, 1, -1};
                RESET_VISITED_LOGIC(Gb); // Reset double counting logic.
                num_captured++; // Increase the number of captured enemies.
                last_captured_pos = indx; // Set the last captured position.
                for (int32_t j = 0; j < 4; j++) {
                    // Check in 4 directions for the chains that need updating.
                    int32_t dpos = indx + Gb->size*dr[j] + dc[j];
                    StoneColors stone = Board(Gb, dpos);
                    int32_t tmp_head;
                    if ((stone == color) && HAS_NOT_BEEN_VISITED(Gb, tmp_head = get_head(Gb, dpos))) {
                        MARK_VISITED(Gb, tmp_head);
                        Gb->chain_liberties[tmp_head]++;
                    }
                }
                // Capture the enemy chain.
                Gb->next_stone[indx] = -1;
                indx = next;
            } while (indx != enemy_head);
        }
    }

    // --- KO DETECTION ---
    Gb->ko_pos = -1; // Default no ko.
    if (num_captured == 1) {
        // The capturing chain must be a single stone.
        if (Gb->chain_size[head] == 1) {
            // That stone must have only one liberty after capture.
            if (Gb->chain_liberties[head] == 1) {
                // That liberty is the capture point.
                Gb->ko_pos = last_captured_pos;
            }
        }
    }
    Gb->num_stones++;
    Gb->turn = color == BLACK ? TURN_WHITE : TURN_BLACK;
    Gb->zobval ^= Gb->ZSideToMove;
    return STATUS_SUCCESS;
}

int32_t check_move_sim(GoBoard* Gb, int32_t position, StoneColors color) {
    // Check for PASS.
    if (position == -1) return STATUS_SUCCESS;

    // Check if position is occupied.
    if (Board(Gb, position) != EMPTY) return STATUS_POSITION_OCCUPIED;

    // Ko violation check.
    if (Gb->ko_pos == position) return STATUS_KO_VIOLATION;

    // Find neighbouring chains.
    int32_t num_friendlies = 0;
    int32_t position_friendlies[4] = {-1, -1, -1, -1};
    int32_t num_enemies = 0;
    int32_t position_enemies[4] = {-1, -1, -1, -1};
    int32_t friend_heads[4];
    int32_t enemy_heads[4];
    find_neighbours(Gb, position, &color, &num_friendlies, position_friendlies, &num_enemies, position_enemies);

    if (num_enemies + num_friendlies != 4) return STATUS_SUCCESS; // Has a liberty therefore it cannot be suicide.

    // Suicide logic.
    unsigned char conditions[2] = {1, 0};
    // Condition: if all neighbour chains have 1 liberty left.
    for (int32_t i = 0; i < num_friendlies; i++) {
        friend_heads[i] = get_head(Gb, position_friendlies[i]);
        if (position_friendlies[i] >= 0) {
            conditions[0] &= (Gb->chain_liberties[friend_heads[i]] == 1);
        }
    }
    // Condition: if at least one enemy chain has 1 liberty left.
    for (int32_t i = 0; i < num_enemies; i++) {
        enemy_heads[i] = get_head(Gb, position_enemies[i]);
        if (position_enemies[i] >= 0) {
            conditions[1] |= (Gb->chain_liberties[enemy_heads[i]] == 1);
        }
    }

    // Suicide check.
    if (conditions[0] && !conditions[1]) return STATUS_SUICIDE_MOVE;

    return STATUS_SUCCESS;
}

int32_t put_move_sim(GoBoard* Gb, int32_t position, StoneColors color) {
    ChangeLog* log = Gb->log;

    // Check for PASS.
    if (position == -1) {
        Gb->ko_pos = -1; // Ko is always cleared on a pass.
        Gb->turn = color == BLACK ? TURN_WHITE : TURN_BLACK;
        Gb->zobval ^= Gb->ZSideToMove;                           
        return STATUS_SUCCESS;
    }
    
    // Check if position is occupied.
    if (Board(Gb, position) != EMPTY) return STATUS_POSITION_OCCUPIED;

    // Ko violation check.
    if (Gb->ko_pos == position) return STATUS_KO_VIOLATION;

    // Find neighbouring chains.
    int32_t num_friendlies = 0;
    int32_t position_friendlies[4] = {-1, -1, -1, -1};
    int32_t num_enemies = 0;
    int32_t position_enemies[4] = {-1, -1, -1, -1};
    int32_t friend_heads[4] = {-1, -1, -1, -1};
    int32_t enemy_heads[4] = {-1, -1, -1, -1};
    find_neighbours(Gb, position, &color, &num_friendlies, position_friendlies, &num_enemies, position_enemies);

    // Suicide logic.
    unsigned char conditions[2] = {1, 0};
    // Condition: if all neighbour chains have 1 liberty left.
    for (int32_t i = 0; i < num_friendlies; i++) {
        friend_heads[i] = get_head(Gb, position_friendlies[i]);
        if (position_friendlies[i] >= 0) {
            conditions[0] &= (Gb->chain_liberties[friend_heads[i]] == 1);
        }
    }
    
    // Condition: if at least one enemy chain has 1 liberty left.
    for (int32_t i = 0; i < num_enemies; i++) {
        enemy_heads[i] = get_head(Gb, position_enemies[i]);
        if (position_enemies[i] >= 0) {
            conditions[1] |= (Gb->chain_liberties[enemy_heads[i]] == 1);
        }
    }

    // Suicide check.
    if (conditions[0] && count_liberties(Gb, position) == 0 && !conditions[1]) {
        return STATUS_SUICIDE_MOVE;
    }

    // Place the stone.
    set_int(log, &Board(Gb, position), color);

    // Merge all friendly neighbours into a single unified chain.
    int32_t head = -1;
    if (num_friendlies > 0) {
        head = friend_heads[0];
        for (int32_t i = 1; i < num_friendlies; i++) {
            head = merge_neighbours_sim(Gb, head, friend_heads[i]);
        }
    }
    // If there where no friendly neighbours, make a new chain.
    if (head == -1) {
        head = initialize_chain_sim(Gb, position);
    } else { // Else enclude the linking stone. (the one that links the num_friendlies chains)
        int32_t tmp_next_head = Gb->next_stone[head];
        set_int(log, &Gb->next_stone[head], position);
        set_int(log, &Gb->next_stone[position], tmp_next_head);
        set_int(log, &Gb->chain_head[position], head);
        set_int(log, &Gb->chain_size[head], Gb->chain_size[head] + 1);
    }
    // And count the liberties of the merged chain.
    set_int(log, &Gb->chain_liberties[head], count_chain_liberties(Gb, head));

    Gb->zobval ^= Gb->zoboard[color-2][position];

    // --- Enemy capturing logic ---
    RESET_VISITED_LOGIC(Gb);
    for (int32_t i = 0; i < num_enemies; i++) {
       if (HAS_NOT_BEEN_VISITED(Gb, enemy_heads[i])) {
            MARK_VISITED(Gb, enemy_heads[i]);
            set_int(log, &Gb->chain_liberties[enemy_heads[i]], Gb->chain_liberties[enemy_heads[i]] - 1);
       }
    }
    int32_t num_captured = 0;
    int32_t last_captured_pos = -1;
    for (int32_t i = 0; i < num_enemies; i++) {
        int32_t ecolor = (color == WHITE) ? BLACK : WHITE;
        int32_t enemy_head = enemy_heads[i];

        // If the chain has already been freed, continue the loop.
        if (enemy_head == -1) continue;
        if (Board(Gb, enemy_head) == EMPTY) continue;
    
        if (Gb->chain_liberties[enemy_head] == 0) {
            int32_t indx = enemy_head;
            do {
                int32_t next = Gb->next_stone[indx];
                if (Board(Gb, indx) != EMPTY) {
                    Gb->zobval ^= Gb->zoboard[ecolor-2][indx];
                    // Remove enemy.
                    set_int(log, &Board(Gb, indx), EMPTY);
                    set_int(log, &Gb->num_stones, Gb->num_stones - 1);
                }
                set_int(log, &Gb->chain_head[indx], -1);
                int32_t dr[4] = {1, -1, 0, 0};
                int32_t dc[4] = {0, 0, 1, -1};
                RESET_VISITED_LOGIC(Gb); // Reset double counting logic.
                num_captured++; // Increase the number of captured enemies.
                last_captured_pos = indx; // Set the last captured position.
                for (int32_t j = 0; j < 4; j++) {
                    // Check in 4 directions for the chains that need updating.
                    int32_t dpos = indx + Gb->size*dr[j] + dc[j];
                    StoneColors stone = Board(Gb, dpos);
                    int32_t tmp_head;
                    if ((stone == color) && HAS_NOT_BEEN_VISITED(Gb, tmp_head = get_head(Gb, dpos))) {
                        MARK_VISITED(Gb, tmp_head);
                        set_int(log, &Gb->chain_liberties[tmp_head], Gb->chain_liberties[tmp_head] + 1);
                    }
                }
                // Capture the enemy chain.
                set_int(log, &Gb->next_stone[indx], -1);
                indx = next;
            } while (indx != enemy_head);
        }
    }

    // --- KO DETECTION ---
    Gb->ko_pos = -1; // Default no ko.
    if (num_captured == 1) {
        // The capturing chain must be a single stone.
        if (Gb->chain_size[head] == 1) {
            // That stone must have only one liberty after capture.
            if (Gb->chain_liberties[head] == 1) {
                // That liberty is the capture point.
                Gb->ko_pos = last_captured_pos;
            }
        }
    }
    Gb->turn = color == BLACK ? TURN_WHITE : TURN_BLACK;
    set_int(log, &Gb->num_stones, Gb->num_stones + 1);
    Gb->zobval ^= Gb->ZSideToMove;
    return STATUS_SUCCESS;
}