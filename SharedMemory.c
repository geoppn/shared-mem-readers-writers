#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>
#include "SharedMemory.h"

int create_shared_memory(key_t key) {
    int shmid = shmget(key, sizeof(SharedData), 0666|IPC_CREAT); // Allocate shared memory for SharedData
    if (shmid == -1) {
        perror("Shared memory creation failed");
        exit(1);
    }
    return shmid;
}

void destroy_shared_memory(int shmid) {
    // Destroy the shared memory segment
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("Shared memory destruction failed");
        exit(1);
    }
}

sem_t* create_semaphore(const char* name) {
    // Initialize a named semaphore and return its pointer
    sem_t* sem = sem_open(name, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("Semaphore initialization failed");
        exit(1);
    }
    return sem;
}

void destroy_semaphore(sem_t* sem, const char* name) {
    // Close and unlink the named semaphore
    if (sem_close(sem) == -1) {
        perror("Semaphore close failed");
    }
    if (sem_unlink(name) == -1) {
        perror("Semaphore unlink failed");
    }
}