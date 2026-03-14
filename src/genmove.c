#include "engine.h"
#include "genmove.h"
#include "move.h"
#include "neighbours.h"
#include "liberties.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void save_undo(GoBoard* sim, Undo* undo) {
    undo->old_turn = sim->turn;
    undo->old_ko_pos = sim->ko_pos;
    undo->old_zobval = sim->zobval;
}

void undo_move(GoBoard* sim, Undo* undo, ChangeLog* log) {
    sim->turn = undo->old_turn;
    sim->ko_pos = undo->old_ko_pos;
    sim->zobval = undo->old_zobval;

    while (log->ptr > 0) {
        log->ptr--;
        *(log->stack[log->ptr].address) = log->stack[log->ptr].value;
    }
}

void add_move(Move* MOVESET, int32_t moveset_size, Move canditate, int32_t Dscore) {
    canditate.score += Dscore;
    MOVESET[moveset_size] = canditate;
}

int32_t evaluate_board(GoBoard* Gb, StoneColors current_color) {
    int32_t score = 0;

    // Directions for neighbor checks. (Territory logic)
    int32_t dr[4] = {1, -1, 0, 0};
    int32_t dc[4] = {0, 0, 1, -1};
    RESET_VISITED_LOGIC(Gb); // To count each chain once.

    for (int32_t i = 0; i < MAX_SIZE(Gb); i++) {
        StoneColors stone = Board(Gb, i);

        if (stone == BORDER) continue;

        if (stone != EMPTY) {
            int32_t head = get_head(Gb, i);

            // Only count each group once.
            if (HAS_NOT_BEEN_VISITED(Gb, head)) {
                MARK_VISITED(Gb, head);

                int32_t chain_sz = Gb->chain_size[head];
                int32_t libs = Gb->chain_liberties[head];

                // Size is the most important.
                int32_t chain_val = chain_sz * 100; 

                // Liberty Adjustments. (Life & Death)
                if (libs == 1) chain_val -= 800;       // Atari / Dead.
                else if (libs == 2) chain_val -= 200;  // Weak.
                else if (libs >= 4) chain_val += 50;   // Strong.

                // Apply score relative to the color.
                if (stone == current_color) score += chain_val;
                else score -= chain_val;

                if (Gb->is_hoshi[i]) {
                    int32_t hoshi_hold_bonus = 300 - (Gb->num_stones * 5);
                    if (hoshi_hold_bonus > 0) {
                        if (stone == current_color) score += hoshi_hold_bonus;
                        else score -= hoshi_hold_bonus;
                    }
                }
            }
        } else {
            int32_t black_neighbours = 0;
            int32_t white_neighbours = 0;
            int32_t border_neighbours = 0;

            // Check 4 neighbours to determine who owns this empty spot.
            for (int32_t k = 0; k < 4; k++) {
                int32_t neighbor_pos = i + dr[k] * Gb->size + dc[k];
                StoneColors s = Board(Gb, neighbor_pos);
                
                if (s == BLACK) black_neighbours++;
                else if (s == WHITE) white_neighbours++;
                else if (s == BORDER) border_neighbours++;
            }

            int32_t territory_val = 0;

            // If touched only by black then it is black territory.
            if (black_neighbours > 0 && white_neighbours == 0) {
                territory_val = 40 + (black_neighbours * 10) + (border_neighbours * 10);
                if (current_color == BLACK) score += territory_val;
                else score -= territory_val;
            // If touched only by white then it is white territory.
            } else if (white_neighbours > 0 && black_neighbours == 0) {
                territory_val = 40 + (white_neighbours * 10) + (border_neighbours * 10);
                if (current_color == WHITE) score += territory_val;
                else score -= territory_val;
            }
        }
    }

    // Add Komi.
    int32_t komi_points = (int32_t)(Gb->komi * 100);
    if (current_color == WHITE) score += komi_points;
    else score -= komi_points;

    return score;
}

// Returns a pointer to the entry if it's a "Hit", otherwise NULL.
TTEntry* probe_tt(uint64_t hash, TTEntry* TransTable) {
    uint64_t index = hash & TT_MASK; // Take the lower bits to fit as an index.
    TTEntry* entry = &TransTable[index];

    if (entry->key == hash) return entry;
    return NULL;
}

void store_tt(TTEntry* TransTable, uint64_t hash, int32_t score, int32_t depth, int32_t alpha, int32_t beta, int32_t best_move) {
    TTEntry* e = &TransTable[hash & TT_MASK]; // The mask shortens the index. (with minimal collisions)
    // Only store if depth given is higher than original stored value.
    if (e->key != 0 && e->depth > depth && e->key == hash) return;
    e->key = hash;
    e->score = score;
    e->depth = depth;
    e->best_move = best_move;

    if (score <= alpha)  e->flag = UPPERBOUND; // Didn't improve alpha so this position is bad.
    else if (score >= beta) e->flag = LOWERBOUND; // This caused a cutoff it's too good for the opponent to allow.
    else e->flag = EXACT; // This is a precise score.
}

int32_t score_move(GoBoard* sim, int32_t pos, StoneColors color) {
    int32_t empty_count = 0, friendly_count = 0, enemy_count = 0, border_count = 0;
    int32_t f_heads[4], e_heads[4];
    int32_t f_heads_cnt = 0, e_heads_cnt = 0;

    int32_t dr[4] = {1, -1, 0, 0};
    int32_t dc[4] = {0, 0, 1, -1};
    int32_t size = sim->size;
    int32_t r = pos / size, c = pos % size;

    for (int32_t j = 0; j < 4; j++) {
        int32_t dpos = pos + size * dr[j] + dc[j];
        StoneColors s = Board(sim, dpos);

        if (s == BORDER) border_count++;
        else if (s == EMPTY) empty_count++;
        else if (s == color) {
            friendly_count++;
            int32_t h = get_head(sim, dpos);
            // Unique head check.
            int32_t unique = 1;
            for (int32_t k = 0; k < f_heads_cnt; k++) if (f_heads[k] == h) unique = 0;
            if (unique) f_heads[f_heads_cnt++] = h;
        } else {
            enemy_count++;
            int32_t h = get_head(sim, dpos);
            // Unique head check.
            int32_t unique = 1;
            for (int32_t k = 0; k < e_heads_cnt; k++) if (e_heads[k] == h) unique = 0;
            if (unique) e_heads[e_heads_cnt++] = h;
        }
    }

    int32_t score = 0;

    int32_t dv[4] = {1, -1, 1, -1};
    int32_t dw[4] = {1, 1, -1, -1};
    if (sim->num_stones < 140) {
        for (int32_t j = 0; j < 4; j++) {
            int32_t dpos = pos + size * dw[j] + dv[j];
            StoneColors s = Board(sim, dpos);
            if (s == color) score += 400;
        }
    }

    // --- STAR POINT LOGIC ---
    if (sim->is_hoshi[pos] && sim->num_stones <= 30) {
        int32_t opening_bonus = 1200 - (sim->num_stones * 40);
        if (opening_bonus > 0) score += opening_bonus;
    }

    // Eye protection.
    if (enemy_count == 0 && (friendly_count + border_count == 4)) return -20000;

    // --- ATTACKING ---
    for (int32_t i = 0; i < e_heads_cnt; i++) {
        int32_t libs = sim->chain_liberties[e_heads[i]];
        int32_t size = sim->chain_size[e_heads[i]];
        
        int32_t attack_val = (size * 5000) / (libs * libs); // Stronger decay.
        score += attack_val;
    }

    // --- DEFENDING ---
    for (int32_t i = 0; i < f_heads_cnt; i++) {
        int32_t libs = sim->chain_liberties[f_heads[i]];
        int32_t size = sim->chain_size[f_heads[i]];
    
        if (libs <= 4) {
            int32_t defense_val = (size * 4000) / (libs * libs);
            score += defense_val;
        }
    }

    // --- CONNECTION (Friendly) ---
    if (f_heads_cnt > 1) {
        int32_t total_friendly_mass = 0;
        for (int32_t i = 0; i < f_heads_cnt; i++) {
            total_friendly_mass += sim->chain_size[f_heads[i]];
        }
        score += 1200 + (total_friendly_mass * 50); 
    }

    // --- SPLITTING (Enemy) ---
    if (e_heads_cnt == 2) {
        int32_t total_enemy_mass = 0;
        for (int32_t i = 0; i < e_heads_cnt; i++) {
            total_enemy_mass += sim->chain_size[e_heads[i]];
        }
        score += 1000 + (total_enemy_mass * 40);
    }

    // If a neighbour friendly group is in Atari, this move is a high priority.
    for (int32_t i = 0; i < f_heads_cnt; i++) {
        if (sim->chain_liberties[f_heads[i]] == 1) {
            score += 3500;
        }
    }

    // --- LIBERTY PREDICTION (Self-Atari) ---
    int32_t predicted_libs = empty_count;
    for (int32_t i = 0; i < f_heads_cnt; i++) {
        predicted_libs += (sim->chain_liberties[f_heads[i]] - 1);
    }

    // If there will be only 1 liberty after moving, it's a "Self-Atari".
    if (predicted_libs == 1) score -= 15000; // Self-Atari (bad).
    else if (predicted_libs == 2) score -= 2000; // Heavy/Cramped.
    else if (predicted_libs >= 4) score += 10; // High Breath / Good Shape.

    // --- SHAPE & TERRITORY ---
    // Distance from center logic.
    int32_t r_dist = (r < size / 2) ? r : size - 1 - r;
    int32_t c_dist = (c < size / 2) ? c : size - 1 - c;
    int32_t edge_dist = (r_dist < c_dist) ? r_dist : c_dist;

    // (Only play here to capture or save)
    if (edge_dist == 1) score -= 1500;
    // (Low value early game)
    else if (edge_dist == 2) score -= 400;
    // (High value for territory/influence)
    else if (edge_dist == 3 || edge_dist == 4) score += 600;

    // --- CLUMPING vs. SHAPE ---
    if (friendly_count == 0) {
        score += 200; // Encourage making new "seeds" on the board.
    } else if (friendly_count >= 3) {
        score -= 800; // Strong penalty for "clumping" into a heavy square block.
    }
    return score;
}

int32_t gen_moves(GoBoard* sim, Move* MOVESET, StoneColors color) {
    int32_t count = 0;
    int32_t size = sim->size;
    int32_t mst_size = 0;

    for (int32_t row = 1; row < size - 1; row++) {
        for (int32_t col = 1; col < size - 1; col++) {
            int32_t pos = POS(row, col, size);

            if (check_move_sim(sim, pos, color) != STATUS_SUCCESS) continue;

            Move move = {pos, score_move(sim, pos, color)};
            add_move(MOVESET, mst_size++, move, 0);
            count++;
        }
    }
    return count;
}

int32_t Important_Moves[MAX_DEPTH+4][2];
ChangeLog GlobalLogs[MAX_DEPTH+4];

int32_t negamax(GoBoard* sim, int32_t depth, int32_t root_depth, int32_t limit, int32_t alpha, int32_t beta, StoneColors color, TTEntry* TransTable) {
    TTEntry* entry = probe_tt(sim->zobval, TransTable);
    if (entry != NULL && entry->depth >= depth) {
        if (entry->flag == EXACT) return entry->score;
        if (entry->flag == LOWERBOUND && entry->score >= beta) return beta;
        if (entry->flag == UPPERBOUND && entry->score <= alpha) return alpha;
    }

    // Stop recursion and evaluate board.
    if (depth == 0) return evaluate_board(sim, color);

    if (root_depth == 5) {
        if (depth == 5 || depth == 4) limit = 60;
        else if (depth == 3 || depth == 2) limit = 20;
        else limit = 5;
    }

    Undo undo;
    ChangeLog* log = &GlobalLogs[depth-1];

    int32_t best_move_this_turn = -1;
    int32_t original_alpha = alpha;
    int32_t tt_best_move = (entry != NULL) ? entry->best_move : -1;
    int32_t best_val = -10000000;

    if (entry != NULL && entry->best_move != -1) {
        // Make the CPU prefetch the TT entry for the position in the next recursion level.
        uint64_t next_hash = sim->zobval ^ sim->zoboard[color-2][entry->best_move];
        __builtin_prefetch(&TransTable[next_hash & TT_MASK], 0, 3);
        log->ptr = 0;
        sim->log = log;
        save_undo(sim, &undo);
        if (put_move_sim(sim, entry->best_move, color) == STATUS_SUCCESS) {
            int32_t score = -negamax(sim, depth-1 , root_depth , limit, -beta, -alpha, (color == BLACK ? WHITE : BLACK), TransTable);
            undo_move(sim, &undo, log);

            if (score > best_val) {
                best_val = score;
                best_move_this_turn = entry->best_move;
            }
            if (best_val > alpha) alpha = best_val;
            if (alpha >= beta) {
                store_tt(TransTable, sim->zobval, beta, depth, original_alpha, beta, entry->best_move);
                return beta;
            }
        }
    }

    if ((Important_Moves[depth-1][0] != -1)) {
        for (int32_t i = 0; i < 2; i++) {
            // Make the CPU prefetch the TT entry for the position in the next recursion level.
            uint64_t next_hash = sim->zobval ^ sim->zoboard[color-2][Important_Moves[depth-1][0]];
            __builtin_prefetch(&TransTable[next_hash & TT_MASK], 0, 3);
            log->ptr = 0;
            sim->log = log;
            save_undo(sim, &undo);

            if (put_move_sim(sim, Important_Moves[depth-1][i], color) != STATUS_SUCCESS) continue;

            int32_t score = -negamax(sim, depth-1, root_depth , limit, -beta, -alpha, (color == BLACK ? WHITE : BLACK), TransTable);

            undo_move(sim, &undo, log);

            if (score > best_val) {
                best_val = score;
                best_move_this_turn = Important_Moves[depth-1][i];
            }
            if (best_val > alpha) alpha = best_val;
            if (alpha >= beta) {
                store_tt(TransTable, sim->zobval, beta, depth, original_alpha, beta, best_move_this_turn);
                return beta;
            }
        }
    }

    // Generate moves.
    Move MOVESET[MAX_SIZE(sim)];
    int32_t n = gen_moves(sim, MOVESET, color);

    if (n == 0) return evaluate_board(sim, color);

    int32_t search_limit = n < limit ? n : limit;
    for (int32_t i = 0; i < n; i++) {
        // Partial selection sort for moves.
        int32_t best_idx = i;
        for (int32_t j = i + 1; j < n; j++) {
            if (MOVESET[j].score > MOVESET[best_idx].score) best_idx = j;
        }
        Move temp = MOVESET[i];
        MOVESET[i] = MOVESET[best_idx];
        MOVESET[best_idx] = temp;

        if (MOVESET[i].pos == tt_best_move) continue;
        if (MOVESET[i].pos == Important_Moves[depth-1][0] || MOVESET[i].pos == Important_Moves[depth-1][1]) continue;

        // If the score is good continue.
        if (i >= search_limit && MOVESET[i].score < 3500) break;

        // Make the CPU prefetch the TT entry for the position in the next recursion level.
        uint64_t next_hash = sim->zobval ^ sim->zoboard[color-2][MOVESET[i].pos];
        __builtin_prefetch(&TransTable[next_hash & TT_MASK], 0, 3);
        log->ptr = 0;
        sim->log = log;
        save_undo(sim, &undo);
        if (put_move_sim(sim, MOVESET[i].pos, color) != STATUS_SUCCESS) continue;

        int32_t score = -negamax(sim, depth-1, root_depth , limit, -beta, -alpha, (color == BLACK ? WHITE : BLACK), TransTable);
        undo_move(sim, &undo, log);

        if (score > best_val) {
            best_val = score;
            best_move_this_turn = MOVESET[i].pos;
        }

        if (best_val > alpha) alpha = best_val;
        if (alpha >= beta) {
            if (score < 2500) {
                Important_Moves[depth-1][1] = Important_Moves[depth-1][0];
                Important_Moves[depth-1][0] = best_move_this_turn;
            }
            break;
        }
    }

    store_tt(TransTable, sim->zobval, best_val, depth, original_alpha, beta, best_move_this_turn);
    return best_val;
}

int32_t run_simulation(GoBoard* sim, StoneColors color, int32_t max_depth, int32_t limit, TTEntry* TransTable) {
    int32_t best_pos = -1;

    // Iterative Deepening.
    for (int32_t depth = 1; depth <= max_depth; depth++) {
        int32_t alpha = -1000000;
        int32_t beta = 1000000;

        // This is the root search.
        negamax(sim, depth, max_depth, limit, alpha, beta, color, TransTable);

        // After the search, the TT will contain the best move for the root.
        TTEntry* root_e = probe_tt(sim->zobval, TransTable);
        if (root_e != NULL && root_e->best_move != -1) {
            best_pos = root_e->best_move;
        }
    }
    return best_pos;
}

int32_t genmove(GoBoard* Gb, GoBoard* sim, StoneColors color, TTEntry* TransTable) {
    sim->size = Gb->size;
    sim->komi = Gb->komi;
    sim->turn = Gb->turn;
    sim->visited_counter = Gb->visited_counter;
    sim->num_stones = Gb->num_stones;
    sim->ko_pos = Gb->ko_pos;
    sim->zobval = Gb->zobval;
    sim->settings.main_time = Gb->settings.main_time;
    sim->settings.byo_yomi = Gb->settings.byo_yomi;
    sim->settings.byo_yomi_stones = Gb->settings.byo_yomi_stones;
    sim->settings.color = Gb->settings.color;
    sim->settings.time = Gb->settings.time;
    sim->settings.stones = Gb->settings.stones;

    memcpy(sim->board, Gb->board, MAX_SIZE(Gb) * sizeof(*Gb->board));
    memcpy(sim->chain_head, Gb->chain_head, MAX_SIZE(Gb) * sizeof(*Gb->chain_head));
    memcpy(sim->next_stone, Gb->next_stone, MAX_SIZE(Gb) * sizeof(*Gb->next_stone));
    memcpy(sim->chain_liberties, Gb->chain_liberties, MAX_SIZE(Gb) * sizeof(*Gb->chain_liberties));
    memcpy(sim->chain_size, Gb->chain_size, MAX_SIZE(Gb) * sizeof(*Gb->chain_size));
    memcpy(sim->visited_array, Gb->visited_array, MAX_SIZE(Gb) * sizeof(*Gb->visited_array));

    // Dynamic Depth & Top Limit Logic.
    int32_t playable = (Gb->size - 2) * (Gb->size - 2);
    float fill = (float)Gb->num_stones / playable;
    int32_t target_depth, target_limit;
 
    if (fill < 0.12f) {
        // Early Game.
        target_depth = MAX_DEPTH;
        target_limit = LIMIT + 8;
    } else if (fill < 0.35f) {
        // Early MidGame.
        target_depth = MAX_DEPTH;
        target_limit = LIMIT + 2;
    } else if (fill < 0.80f) {
        // MidGame & Late MidGame.
        target_depth = MAX_DEPTH;
        target_limit = LIMIT + 5;
    } else {
        // EndGame.
        target_depth = MAX_DEPTH + 4;
        target_limit = 6;
    }

    if (sim->settings.time <= 1) {
        // Security: Do not lose from time.
        target_depth = 3;
        target_limit = 20;
    }

    for (int32_t i = 0; i < target_depth; i++) { // Initialize important moves.
        Important_Moves[i][0] = -1;
        Important_Moves[i][1] = -1;
    }

    int32_t best_pos = -1;

    if (fill <= 0.90f) best_pos = run_simulation(sim, color, target_depth, target_limit, TransTable);

    if (put_move(Gb, best_pos, color) != STATUS_SUCCESS) {
        best_pos = -1;
        put_move(Gb, best_pos, color);
    }
    return best_pos;
}