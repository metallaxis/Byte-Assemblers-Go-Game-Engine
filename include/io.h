#ifndef IO_H
#define IO_H

#include "engine.h"

#include <stdio.h>

/*
    @brief Passively read user input
    @param msgptr Pointer to the message string 
    @param n Pointer to the size of the message
    @param stream Pointer to the file from which it reads
    @return Message length
*/
long passive_read(char **msgptr, size_t *n, FILE *stream);

/*
    @brief Prints the message according to the code and an additional msg
    @param Gb Pointer to the GoBoard
    @param code Status code (The below works ONLY FOR SUCCESS AND FAILURE)
    @param msg Possible additional message string ("" for empty)
*/
void print(GoBoard* Gb, Status code, char* msg);

#endif // IO_H