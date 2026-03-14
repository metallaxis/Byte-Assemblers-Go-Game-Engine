
#ifndef MOVE_H
#define MOVE_H

#include "engine.h"

/*
    @brief Undo the last move
    @param Gb Pointer to the GoBoard
    @param boards Pointer to the last board
*/
void undo_previous_move(GoBoard* Gb, Boards** boards);

/*
    @brief Place a move on the Go board
    @param Gb Pointer to the GoBoard
    @param position The position you want to place the stone
    @param color Color of the stone (BLACK for black, WHITE for white)
    @return Status code indicating success or type of error
*/
int32_t put_move(GoBoard* Gb, int32_t position, StoneColors color);

/*
    @brief Check if a move on the Go board is valid
    @param Gb Pointer to the GoBoard
    @param position The position you want to check if you can place
    @param color Color of the stone (BLACK for black, WHITE for white)
    @return Status code indicating success or type of error
*/
int32_t check_move_sim(GoBoard* Gb, int32_t position, StoneColors color);

/*
    @brief Place a move on the Go board
    @note This is for the sim Board
    @param Gb Pointer to the GoBoard
    @param position The position you want to place the stone
    @param color Color of the stone (BLACK for black, WHITE for white)
    @return Status code indicating success or type of error
*/
int32_t put_move_sim(GoBoard* Gb, int32_t position, StoneColors color);

#endif // MOVE_H