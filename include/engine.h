#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>

// To refer to a given position in the GoBoard.
#define POS(row, col, w) ((row) * (w) + (col)) 
// To refer to a given stone in the 1D GoBoard (pseudo-2D).
#define BOARD(Gb, x, y) (Gb->board[POS(x, y, (Gb->size))])
#define Board(Gb, indx) (Gb->board[indx])

#define RESET_VISITED_LOGIC(Gb) (Gb->visited_counter++)
#define HAS_NOT_BEEN_VISITED(Gb, dpos) (Gb->visited_array[dpos] != Gb->visited_counter)
#define MARK_VISITED(Gb, dpos) (Gb->visited_array[dpos] = Gb->visited_counter)

#define MAX_SIZE(Gb) (Gb->size * Gb->size)
#define MAX_BOARD_SQUARES 729

#define TT_BITS 25
#define TT_SIZE (1 << TT_BITS)
#define TT_MASK (TT_SIZE - 1)

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// Status codes.
typedef enum {
    STATUS_WRONG_TURN = -5,
    STATUS_KO_VIOLATION = -4,
    STATUS_SUICIDE_MOVE = -3,
    STATUS_POSITION_OCCUPIED = -2,
    STATUS_OUT_OF_BOUNDS = -1,
    STATUS_SUCCESS = 0,
    STATUS_FAILURE = 1,
    STATUS_MEMORY_ALLOC_FAILED = 2,
    STATUS_SHOW_BOARD = 3,
    STATUS_QUIT = 4,
    STATUS_UNKNOWN = 5
} Status;

// Players Turn.
typedef enum {
    TURN_BLACK = 0,
    TURN_WHITE = 1
} Turn;

// Definitions for the stones on the board.
typedef enum {
    EMPTY = 0,
    BORDER = 1,
    WHITE = 2,
    BLACK = 3
} StoneColors;

typedef struct {
    // Time settings.
    int32_t main_time;        // Main time measured in seconds.
    int32_t byo_yomi;         // Byo yomi time measured in seconds.
    int32_t byo_yomi_stones;  // Number of stones per byo yomi period.

    // Time left.
    int8_t color;             // Color for which the information applies.
  	int32_t time;             // Number of seconds remaining.
  	int32_t stones;           // Number of stones remaining.
} Settings;

// A single change to an integer value in memory.
typedef struct {
    int32_t* address;    // Memory address of the change that is about to happen.
    int32_t value;       // The value before the change.
} Change;

// The stack.
typedef struct {
    Change stack[4096];  // A stack of 4096 changes that can be stored.
    int32_t ptr;         // Pointer to the top of the stack.
} ChangeLog;

typedef struct {
    ChangeLog* log;               // Log the changes that are about to happen in the next move.
    int32_t size;                 // e.g. 21 (for a board of 19x19 places for stones)
    float komi;                   // e.g. 0.5
    Turn turn;                    // e.g. Black. The turn of the current player.

    int32_t* board;               // The 1D array carrying the stones (0 for empty, 1 for white stones, 2 for black stones and 3 for borders.)

    // Chains.
    int32_t* chain_head;          // Location of every chain head on the board.
    int32_t* next_stone;          // Location of the next stone in the same chain.
    // Chain Properties (Belong only to the chains head stone).
    int32_t* chain_liberties;     // Number of liberties for a specific chain.
    int32_t* chain_size;          // Number of stones of a specific chain.

    // Optimizations.
    uint64_t visited_counter;     // Track the visited value of the entire board.
    uint64_t* visited_array;      // Track the visited value of each position.

    int32_t* is_hoshi;            // Hoshi board for the star point system.

    int32_t num_stones;           // Number of stones on the board.
    int32_t ko_pos;               // Ko Tracking.

    uint64_t** zoboard;           // Values used to hash the board.
    uint64_t zobval;              // Hashed board.
    uint64_t ZSideToMove;         // Unique value for storing the information of "who's playing? (black or white) to xor with the zobval"
    
    Settings settings;            // Timing settings.
} GoBoard;

typedef struct Boards {
    Turn turn;                    // e.g. Black. The turn of the current player.

    int32_t* board;                // The 1D array carrying the stones (0 for empty, 1 for white stones, 2 for black stones and 3 for borders.)

    // Chains.
    int32_t* chain_head;          // Location of every chain head on the board.
    int32_t* next_stone;          // Location of the next stone in the same chain.
    // Chain Properties (Belong only to the chains head stone).
    int32_t* chain_liberties;     // Number of liberties for a specific chain.
    int32_t* chain_size;          // Number of stones of a specific chain.

    int32_t num_stones;           // Number of stones on the board.
    int32_t ko_pos;               // Ko Tracking.

    uint64_t zobval;              // Hashed board.

    struct Boards* pre_board;     // Pointer to the previous board.
} Boards;

/*
    @brief Add a specific change to the changelog
    @param log Pointer to the Changelog
    @param ptr Pointer to the memory where the value will be changed
    @param new_val The new value of the changed value
*/
static inline void set_int(ChangeLog* log, int32_t* ptr, int32_t new_val) {
    if (*ptr == new_val) return;
    log->stack[log->ptr].address = (int32_t*)(ptr);
    log->stack[log->ptr].value = *(ptr);
    log->ptr++;
    *ptr = new_val;
}

/*
    @brief Restore the previous Go board from the linked list
    @param Gb Pointer to the Go board
    @param boards Pointer to the pointer of the last board
*/
void restore_board(GoBoard* Gb, Boards** boards);

/*
    @brief Insert the current Go board to the linked list
    @param Gb Pointer to the Go board
    @param boards Pointer to the pointer of the last board
*/
void insert_board(GoBoard* Gb, Boards** boards);

/*
    @brief Removes the last Go board from the linked list
    @param boards Pointer to the pointer of the last board
*/
void remove_last(Boards** boards);

/*
    @brief Initialize a new chain
    @param Gb Pointer to the GoBoard
    @param position Position of the first stone
    @return The head of a newly created chain
*/
int32_t initialize_chain(GoBoard* Gb, int32_t position);

/*
    @brief Initialize a new chain
    @note This is for the sim Board
    @param Gb Pointer to the GoBoard
    @param position Position of the first stone
    @return The head of a newly created chain
*/
int32_t initialize_chain_sim(GoBoard* Gb, int32_t position);

/*
    @brief Initialize the Zobrist System
    @param Gb Pointer to the pointer to the GoBoard
*/
int32_t init_zobrist_system(GoBoard* Gb);

#endif // ENGINE_H