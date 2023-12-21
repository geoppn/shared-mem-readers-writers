#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "SharedMemory.h" 

int main(int argc, char *argv[]) {
    struct timespec start_time;
    struct timespec delay_start;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    clock_gettime(CLOCK_MONOTONIC, &delay_start);

    // OPEN THE SEMAPHORES WITHIN THIS PROGRAM
    sem_t *block_sems[NUM_BLOCKS];
    sem_t *mutex;
    char sem_name[20];

    for (int i = 0; i < NUM_BLOCKS; i++) {
        sprintf(sem_name, "/block_sem_%d", i);
        block_sems[i] = sem_open(sem_name, 0);
    }
    mutex = sem_open("/mutex", 0);

    // GET ARGV VARIABLES AND CONVERT THEM TO THE CORRECT DATA TYPES (IF NEEDED)
    char *filename = argv[1];
    int recid = atoi(argv[2]);
    int value = atoi(argv[3]);
    int dw = atoi(argv[4]);
    int shmid = atoi(argv[5]);

    // ATTACH THE SHARED MEMORY TO THIS PROGRAM
    SharedData *data = (SharedData*) shmat(shmid, NULL, 0);  


    // OPEN THE FILE (WITH WRITING PERMS TOO)
    FILE *file = fopen(filename, "r+");
    fseek(file, 0, SEEK_END); 
    long filesize = ftell(file); 
    int max_records = filesize / sizeof(Record); 
    int block_index = recid / (max_records/NUM_BLOCKS);

    struct timespec delay_end; // DELAY END TIME

    sem_wait(block_sems[block_index]);
    clock_gettime(CLOCK_MONOTONIC, &delay_end);
    // SEEK TO THE APPROPRIATE RECORD
    fseek(file, recid * sizeof(Record), SEEK_SET);

    // READ THE RECORD
    Record rec;
    fread(&rec, sizeof(Record), 1, file);

    // ADD THE VALUE
    rec.balance += value;

    // SEEK BACK TO THE SAME RECORD
    fseek(file, recid * sizeof(Record), SEEK_SET);

    // WRITE THE RECORD BACK
    fwrite(&rec, sizeof(Record), 1, file);


    // PRINT THE ALTERED RECORD
    printf("Writer with PID: %d has updated the record: ID: %d, Surname: %s, Name: %s, New Balance: %d\n", getpid(), rec.customerID, rec.lName, rec.fName, rec.balance);


    // HANDLE SLEEPING
    int sec = rand() % (dw + 1);
    sleep(sec);

    sem_post(block_sems[block_index]);
    printf("writer posted for record id %d\n", recid);

    // CLOSE THE FILE
    fclose(file);

    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double total_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9; // CALCULATE TIME AND CONVERT TO SECONDS
    double delay = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    
    // WRITE ALL INFO TO SHARED MEMORY
    sem_wait(mutex); // LOCK THE SHARED MEMORY

    for (int i = 0; i < NUM_BLOCKS; i++) { 
        if (data->writer_times[i] == -1.0) { 
            data->writer_times[i] = total_time;
            break;
        }
    }
    data->completed_writers++; 
    data->processed_records++; 
    if (delay > data->maxdelay) { 
        data->maxdelay = delay;
    }
    sem_post(mutex); // UNLOCK THE SHARED MEMORY  

    // CLOSE THE SEMAPHORES
    for (int i = 0; i < NUM_BLOCKS; i++) {
        sem_close(block_sems[i]);
    }
    sem_close(mutex);

    // DETACH THE SHARED MEMORY FROM THIS PROGRAM
    shmdt(data);
}