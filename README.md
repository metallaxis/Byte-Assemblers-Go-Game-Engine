# Go Game Engine

## How it works
This software creats an Engine for the game Go (the chinese rules). It uses the Go Text Protocol (GTP v2.0) as a way to interact witth the engine, it implements a wide variaty of its options.

## Example
```sh
$ cat test/uni-example.txt 
boardsize 5
komi 0.5
clear_board
showboard
play black C3
play white C4
play black B4
play white B3
play black B2
showboard
play white D3
play black A3
showboard
play white D5
play black E4
showboard
play white C2
play black B3
showboard
play white D1
play black D4
showboard
play white E3
play black C5
showboard
play white E2
play black B1
showboard
play white C1
play black PASS
showboard
play white PASS
final_score
quit
```

```
$ ./goteam < test/uni-example.txt 
= 

= 

= 

=
   A B C D E 
 5 . . . . . 5
 4 . . . . . 4
 3 . . . . . 3
 2 . . . . . 2
 1 . . . . . 1
   A B C D E
= 

= 

= 

= 

= 

=
   A B C D E 
 5 . . . . . 5
 4 . X O . . 4
 3 . O X . . 3
 2 . X . . . 2
 1 . . . . . 1
   A B C D E
= 

= 

=
   A B C D E 
 5 . . . . . 5
 4 . X O . . 4
 3 X . X O . 3
 2 . X . . . 2
 1 . . . . . 1
   A B C D E
= 

= 

=
   A B C D E 
 5 . . . O . 5
 4 . X O . X 4
 3 X . X O . 3
 2 . X . . . 2
 1 . . . . . 1
   A B C D E
= 

= 

=
   A B C D E 
 5 . . . O . 5
 4 . X O . X 4
 3 X X X O . 3
 2 . X O . . 2
 1 . . . . . 1
   A B C D E
= 

= 

=
   A B C D E 
 5 . . . O . 5
 4 . X O X X 4
 3 X X X O . 3
 2 . X O . . 2
 1 . . . O . 1
   A B C D E
= 

= 

=
   A B C D E 
 5 . . X O . 5
 4 . X . X X 4
 3 X X X O O 3
 2 . X O . . 2
 1 . . . O . 1
   A B C D E
= 

= 

=
   A B C D E 
 5 . . X O . 5
 4 . X . X X 4
 3 X X X O O 3
 2 . X O . O 2
 1 . X . O . 1
   A B C D E
= 

= 

=
   A B C D E 
 5 . . X O . 5
 4 . X . X X 4
 3 X X X O O 3
 2 . X O . O 2
 1 . X O O . 1
   A B C D E
= 

= B+5.5
```

## Cloning
```sh
$ git clone https://github.com/progintro/hw3-byte-assemblers.git
```

## Compiling
After cloning the repository run the following command: `$ make`

## Running
Run the file that was created from compilation by executing: `$ ./goteam`.
Then you can start playing the game with the moves.
Execute `list_commands` to find the possible commands that can be executed.

## Implementation in C
###  (engine.c)
This file serves as the application entry point and state persistence layer for the Go engine. It manages the primary execution loop, responsible for reading and parsing incoming protocol commands and dispatching them to the engine’s core logic. Additionally, it implements a stack-based history system using a linked list of board snapshots, allowing the engine to store, restore, or discard previous game states to support robust undo operations and game-tree exploration.

### Mode Selection
This component serves as the Command Processor for the Go engine, implementing the Go Text Protocol (GTP v2.0) interface to facilitate communication with the Command Line Interface (CLI). It manages the lifecycle of the game state through robust dynamic memory allocation and an integrated history system that supports move undos via a linked list of board snapshots. The module acts as a central dispatcher, routing protocol commands to specific engine subsystems for move generation, move validation, and automated scoring, while ensuring consistent state synchronization between the primary game board and simulation instances used for AI look-ahead.

### Input / Output
This module serves as the Input/Output (I/O) layer, bridging the engine's internal logic with the external world through the Go Text Protocol (GTP). It includes a robust, dynamically allocating input parser (passive_read) that sanitizes incoming commands by stripping comments and handling variable length strings. Additionally, the print function standardizes the engine's feedback, translating status codes into protocol compliant responses and generating a detailed ASCII board visualization. This visualizer accurately maps coordinates including the traditional skipping of the letter "I" and renders stone positions to provide a human-readable interface for debugging and play.

### Core Game Logic / Board Logic & Ruleset
1. Board Representation

The board is implemented as a 1D array of StoneColors (enum struct).
Pseudo-2D Mapping: While Go is played on a 2D grid, using a 1D array improves speed because accessing adjacent memory addresses is faster for the CPU than jumping between pointers in a 2D array.

Padding (Border Strategy): The StoneColors enum includes a BORDER type. By surrounding the board with a "ring" of border stones, the engine avoids expensive boundary checks (e.g., if (x > 0 && x < size)) during recursive searches or neighbor lookups.

2. Chain Management (The "Stone Group" Logic)

This engine avoids expensive algorithms by using two coordinated structures:
  A. Weighted Union-Find with Path Halving

  The chain_head array tracks which group a stone belongs to.
  get_head(position): This function finds the "representative" stone of a chain. It uses Path Halving, a technique where every lookup flattens the tree structure by a given amount (halving its tree-structure untill its perfectly flat).

  Efficiency: This ensures that finding a chain's head is nearly O(1) (amortized), even on large boards.

  B. Circular Linked Lists

  The next_stone array allows the engine to iterate through every stone in a chain without searching the whole board.

  Merging: When two chains are joined by a new stone, the engine merges their circular lists by simply swapping two pointers. This is an O(1) operation.

3. Liberty Tracking & Capturing

Incremental Updates

Instead of re-calculating liberties for the entire board every turn, the engine uses incremental updates:

    When a stone is placed, it identifies neighboring enemy heads.

    It decrements their liberty counts: Gb->chain_liberties[enemy_head]--.

    If a count hits 0, the engine iterates through that enemy's circular list (next_stone), removes the stones, and increments the liberties of any adjacent friendly chains.

Fast Capture Logic

Because the engine knows exactly which stones are in a captured chain (via the circular list), it can clear a group and update the board in O(K) time, where K is the number of stones in that specific group.

4. Move validity checking

A. The Suicide Rule

A move is suicide if the stone placed has no liberties and doesn't capture an enemy.

  check_move looks at the 4 neighbours.
  It checks if a neighboring friendly chain has liberties == 1. If all neighbours are "in atari" (1 liberty) and the current spot has no empty neighbours, it’s suicide unless a neighboring enemy chain also has liberties == 1 (meaning we capture them first),
  O(1) lookup because the liberty count is already stored in Gb->chain_liberties[head].


  B. The Ko Rule

  The Ko rule prevents the game from looping forever.

  The Logic: When a move captures exactly one stone and leaves the player with exactly one liberty in a specific way, the engine sets Gb->ko_pos.

  check_move simply does an integer comparison: if (position == Gb->ko_pos) return STATUS_KO_VIOLATION;

5. Key Optimizations
The "Versioned" Visited Array

Standard algorithms use a boolean visited array and memset it to zero every time (This is O(N)), Our engine uses:

    visited_counter: A 64-bit integer.
    visited_array: Stores the counter value at which a cell was last visited.

To "clear" the visited status of the entire board, we simply call RESET_VISITED_LOGIC(Gb),which increments the visited_counter. A cell is only considered visited if visited_array[pos] == visited_counter.Therefore we avoid tuple counting liberties,chain "leaders" and more with a O(1) operation.

### Simulations / GenMove
This module implements the core intelligence of the Go engine, utilizing a Negamax search algorithm (a variant of Minimax that simplifies implementation by leveraging the fact that max(a,b)=-min(-a,-b)) to perform recursive lookahead simulations to a depth of 3.

To maintain efficiency against Go's vast branching factor, the engine employs Alpha-Beta pruning, a search optimization that "cuts off" branches in the game tree that are proven to be worse than previously examined moves, alongside a Top n branching limit (out of all legal moves) and move ordering via partial selection sort.

The decision making is driven by a heuristic evaluation engine (score_move) that calculates weights based on tactical combat (using non linear scaling (sz2) to prioritize capturing large enemy chains) and group connectivity, which assigns massive bonuses for merging friendly groups and "saving" groups currently in Atari. 

Furthermore, the engine incorporates Liberty Prediction to penalize "Self-Atari" blunders and positional strategy to reward moves on the 3rd and 4th lines while avoiding inefficient "clumping." To facilitate this high-speed search, the module features a specialized state management system (save_undo/undo_move) that uses bulk memory operations to snapshot and restore board structures, liberties, and Ko positions, allowing the AI to evaluate thousands of hypothetical move sequences per second.

## Sources / Extra Info
[Go Game](https://en.wikipedia.org/wiki/Go_(game))<br>
[Go Rules](https://en.wikipedia.org/wiki/Rules_of_Go)<br>
[Chinese Go Rules](https://www.cs.cmu.edu/~wjh/go/rules/Chinese.html)<br>
Co-Authors Published Version - TO BE ADDED (When they make a copy of this repo in their public userprofile)
[HW3 Webpage](https://progintro.github.io/assets/pdf/hw3.pdf)<br>
