#ifndef MODE_H
#define MODE_H

#include "engine.h"
#include "genmove.h"

// A mode system to excecute each command with respect to a switch case.
typedef enum {
    MODE_UNKNOWN = -1,
    MODE_BOARD_SIZE = 0,
    MODE_KOMI = 1,
    MODE_CLEAR_BOARD = 2,
    MODE_SHOW_BOARD = 3,
    MODE_GENMOVE = 4,
    MODE_PLAY_MOVE = 5,
    MODE_UNDO_MOVE = 6,
    MODE_LIST_COMMANDS = 7,
    MODE_PROTOCOL_VERSION = 8,
    MODE_NAME = 9,
    MODE_VERSION = 10,
    MODE_FINAL_SCORE = 11,
    MODE_TIME_SETTINGS = 12,
    MODE_TIME_LEFT = 13,
    MODE_QUIT = 14,
    MODE_KNOWN_COMMAND = 15
} Modes;

/*
    @brief Initiliaze the memory that will be used
    @param Gb Pointer to the GoBoard
    @param size The size of the board to create
    @return Status code indicating success or type of error
    
*/
int32_t init_engine_memory(GoBoard* Gb, int32_t size);

/*
    @brief Initiliaze the hoshi table
    @param Gb Pointer to the GoBoard
    @return Status code indicating success or type of error
*/
int32_t init_star_points(GoBoard* Gb);

/*
    @brief Free the intialized memory
    @param Gb Pointer to the GoBoard
*/
void free_engine_memory(GoBoard* Gb);

/*
    @brief Execute the correct mode
    @param mode Mode to use
    @param Gb Pointer to the GoBoard
    @param sim Pointer to the Simulation GoBoard
    @param TransTable Pointer to the Pointer to the TTEntry
    @param boards Pointer to the Pointer of the last board
    @param p User string input after skipping first argument
    @return Status code indicating success or type of error
*/
int32_t use(Modes mode, GoBoard* Gb, GoBoard* sim, TTEntry** TransTable, Boards** boards, char* p);

#endif // MODE_H