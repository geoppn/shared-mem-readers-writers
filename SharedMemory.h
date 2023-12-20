#pragma once

#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include "RECORD.h"

#define NUM_BLOCKS 20 // CHANGING TO A HIGHER VARIABLE IS NOT RECOMMENDED

typedef struct SharedData{
    double writer_times[NUM_BLOCKS]; // ARRAY OF THE ELAPSED TIMES OF THE WRITERS
    double reader_times[NUM_BLOCKS]; // ARRAY OF THE ELAPSED TIMES OF THE READERS
    double maxdelay; // MAXIMUM DELAY 
    int completed_writers; // COUNTER FOR COMPLETED WRITERS
    int completed_readers; // COUNTER FOR COMPLETED READERS
    int processed_records; // COUNTER FOR PROCESSED RECORDS
} SharedData;

int create_shared_memory(key_t);
void destroy_shared_memory(int);
sem_t* create_semaphore(const char*);
void destroy_semaphore(sem_t* sem,const char*);
