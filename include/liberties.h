#ifndef LIBERTIES_H
#define LIBERTIES_H

#include "engine.h"

/*
    @brief Count the liberties of a single stone at position pos
    @param Gb Pointer to the GoBoard
    @param pos Position of the stone
    @return Number of liberties for the stone
*/
int32_t count_liberties(GoBoard* Gb, int32_t pos);

/*
    @brief Count the total liberties of a chain (without double-counting)
    @param Gb Pointer to the GoBoard
    @param head_pos position of the head stone
    @return Total number of unique liberties for the chain
*/
int32_t count_chain_liberties(GoBoard* Gb, int32_t head_pos);

/*
    @brief Approximate the total liberties of a chain (without double-counting)
    @param Gb Pointer to the GoBoard
    @param head_pos position of the head stone
    @param num_max_liberties The max number of liberties to calculate and break if reached
    @return Approximate number of unique liberties for the chain
*/
int32_t approx_chain_liberties(GoBoard* Gb, int32_t head_pos, int32_t num_max_liberties);

#endif // LIBERTIES_H