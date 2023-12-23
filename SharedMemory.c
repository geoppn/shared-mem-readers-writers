#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>
#include "SharedMemory.h"

int create_shared_memory(key_t key) {
    int shmid = shmget(key, sizeof(SharedData), 0666|IPC_CREAT);
    if (shmid == -1) {
        perror("Shared memory creation failed");
        exit(1);
    }
    return shmid;
}

void destroy_shared_memory(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("Shared memory destruction failed");
        exit(1);
    }
}

sem_t* create_semaphore(const char* name) {
    sem_t* sem = sem_open(name, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("Semaphore initialization failed");
        exit(1);
    }
    return sem;
}

void destroy_semaphore(sem_t* sem, const char* name) {
    if (sem_close(sem) == -1) {
        perror("Semaphore close failed");
    }
    if (sem_unlink(name) == -1) {
        perror("Semaphore unlink failed");
    }
}

void start_read(sem_t *reader_sem, sem_t *writer_sem, SharedData *shared, int block_index){
    sem_wait(reader_sem);
    shared->block_readers[block_index]++;
    if (shared->block_readers[block_index] == 1) { // IF THIS IS THE FIRST READER (WITHIN THIS BLOCK), BLOCK WRITERS
        sem_wait(writer_sem); 
    }
    sem_post(reader_sem);
}

void end_read(sem_t *reader_sem, sem_t *writer_sem, SharedData *shared, int block_index){
    sem_wait(reader_sem);
    shared->block_readers[block_index]--;
    if (shared->block_readers[block_index] == 0) { // IF THIS IS THE LAST READER, UNBLOCK WRITERS
        sem_post(writer_sem); 
    }
    sem_post(reader_sem);
}