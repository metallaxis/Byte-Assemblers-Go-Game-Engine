#ifndef GENMOVE_H
#define GENMOVE_H

#include "engine.h"

#define MAX_DEPTH 5
#define LIMIT 20

typedef enum {
    UPPERBOUND = 0,
    LOWERBOUND = 1,
    EXACT = 2
} BOUNDS;

typedef struct {
    Turn old_turn;           // e.g. Black. The turn of the current player.

    int32_t old_ko_pos;      // Ko Tracking.
    uint64_t old_zobval;     // Hashed board.
} Undo;

typedef struct {
    int32_t pos;
    int32_t score; // Heuristic score.
} Move;

typedef struct {
    uint64_t key;       // 8 bytes: The unique Zobrist Hash
    int32_t score;      // 4 bytes: The evaluation score
    int16_t best_move;  // 2 bytes: The position to try first
    uint8_t depth;      // 1 byte: How deep we searched this node
    uint8_t flag;       // 1 byte: EXACT, LOWERBOUND, or UPPERBOUND
} TTEntry;

/*
    @brief Generate a move to place on the Go board
    @param Gb Pointer to the GoBoard
    @param sim Pointer to the pointer to the Simulation GoBoard
    @param color Color of the stone (BLACK for black, WHITE for white)
    @param TransTable Pointer to the TTEntity
    @return The position of the placed stone
*/
int32_t genmove(GoBoard* Gb, GoBoard* sim, StoneColors color, TTEntry* TransTable);

#endif // GENMOVE_H