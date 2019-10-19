//Author:   Colby Ackerman
//Class:    CS4760 Operating Systems
//Assign:   #4
//Date:     10/22/19
//-----------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sharedMem.h"

int main(int arg, char* argv[]) {

    char convertString[100];

    Clock s;
    s.seconds = 0;
    s.nanoseconds = 1;

    sprintf(convertString, "%d", s.nanoseconds);
    fprintf(stderr, "nanoseconds: %s\n", convertString);

    return 0;
}