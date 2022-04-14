//
// Created by eric white on 1/4/22.
//
#include "reln.h"
#include "chvec.h"
#include <stdio.h>

int main(int argc, char **argv) {
    char cv[] = "0,1:0,2:1,1:1,2:2,2";
    if (newRelation("test", 3, 4, 0,cv) != 0) {
        printf ("error happened");
        return 1;
    }

    Reln r = openRelation("test", "r");
    tupleHash(r, "111,222,333");
    // printChVec(chvec(r));
    return 0;
}