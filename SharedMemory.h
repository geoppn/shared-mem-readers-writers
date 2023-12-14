#pragma once

#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include "RECORD.h"

#define MAX_READERS 50
#define MAX_WRITERS 50

typedef struct SharedData{
    double writer_times[MAX_WRITERS]; // ARRAY OF THE ELAPSED TIMES OF THE WRITERS
    double reader_times[MAX_READERS]; // ARRAY OF THE ELAPSED TIMES OF THE READERS
    double maxdelay; // MAXIMUM DELAY 
    int completed_writers; // COUNTER FOR COMPLETED WRITERS
    int completed_readers; // COUNTER FOR COMPLETED READERS
    int processed_records; // COUNTER FOR PROCESSED RECORDS
} SharedData;

int create_shared_memory(key_t key);
void destroy_shared_memory(int shmid);
sem_t* create_semaphore();
void destroy_semaphore(sem_t* sem);
