//
// Created by konrad on 2/20/21.
//

#ifndef SMOKERS_SEM_H
#define SMOKERS_SEM_H

#include <stdio.h>
#include <sys/sem.h>

/**
 * Macro for proberen operation on a mutex.
 */
#define lock(sem) semcall(sem, -1)
/**
 * Macro for verhogen operation on a mutex.
 */
#define unlock(sem) semcall(sem, 1)

/**
 * Simplified API for the IPC System V's semaphores.
 * @param sem_id semaphore identifier;
 * @param op value to modify the semaphore.
 */
static void semcall(int sem_id, int op) {
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = op;
    sb.sem_flg = 0;
    if (semop(sem_id, &sb, 1) == -1)
        perror("semop");
}

/**
 * Simplified API to initialise an IPC System V's semaphore with a given value.
 * @param sem_id semaphore identifier;
 * @param init_val initial value for the semaphore.
 */
static void seminit(int sem_id, int init_val) {
    semctl(sem_id, 0, SETVAL, init_val);
}

#endif //SMOKERS_SEM_H
