#ifndef NEIGHBOURS_H
#define NEIGHBOURS_H

#include "engine.h"

/*
    @brief Get the head of current chain
    @details This is a static inline to be faster
    @param Gb Pointer to the GoBoard
    @param position The position of the head you want find the head of
    @return The head position index
*/
static inline int32_t get_head(GoBoard* Gb, int32_t position) {
    int32_t* heads = Gb->chain_head;
    while (heads[position] != position) {
        // Move to the new parent (which was the grandparent)
        position = heads[position];
    }
    return position;
}

/*
    @brief Find neighbouring chains of the same and opposite color
    @param Gb Pointer to the GoBoard
    @param position The position of the head you want find the head of
    @param color Pointer to the color of the stone at position
    @param num_friendlies Pointer to store the number of friendly neighbours found
    @param position_friendlies Array to store positions of friendly neighbours
    @param num_enemies Pointer to store the number of enemy neighbours found
    @param position_enemies Array to store positions of enemy neighbours
*/
void find_neighbours(GoBoard* Gb, int32_t position, StoneColors* color,
    int32_t* num_friendlies, int32_t* position_friendlies, int32_t* num_enemies,
    int32_t* position_enemies);

/*
    @brief Merge two neighbouring chains into one
    @note Does not calculate the liberties of the merged chain.
    @param Gb Pointer to the GoBoard
    @param pos_i Position of the first chain's stone
    @param pos_j Position of the second chain's stone
    @param i Index of the neighbour being merged
    @return The head of the merged chain
*/
int32_t merge_neighbours(GoBoard* Gb, int32_t pos_i, int32_t pos_j);

/*
    @brief Merge two neighbouring chains into one
    @note This is for the sim Board
    @note Does not calculate the liberties of the merged chain.
    @param Gb Pointer to the GoBoard
    @param pos_i Position of the first chain's stone
    @param pos_j Position of the second chain's stone
    @param i Index of the neighbour being merged
    @return The head of the merged chain
*/
int32_t merge_neighbours_sim(GoBoard* Gb, int32_t pos_i, int32_t pos_j);

#endif // NEIGHBOURS_H