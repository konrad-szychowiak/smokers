//
// Created by konrad on 2/20/21.
//

#ifndef SMOKERS_WAIT_H
#define SMOKERS_WAIT_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define WAIT_TIME_MIN 1
#define WAIT_TIME_MAX 5

int randomized(int min, int max) {
    return rand() % (max - min + 1) + min;
}

void randomised_sleep(const int id) {
    int time = randomized(WAIT_TIME_MIN, WAIT_TIME_MAX);
    printf("\t[%3x] sleeps for %u sec\n", id, time);
    sleep(time);
}


#endif //SMOKERS_WAIT_H
