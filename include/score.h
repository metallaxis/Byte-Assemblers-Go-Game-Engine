#ifndef SCORE_H
#define SCORE_H

#include "engine.h"

/*
    @param Calculate the score of the current board position
    @param Gb Pointer to the Go board
    @return The score of the current board position
*/
float calculate_score(GoBoard* Gb);

#endif // SCORE_H