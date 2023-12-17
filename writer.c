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
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // OPEN THE SEMAPHORES WITHIN THIS PROGRAM
    sem_t *reader_sem = sem_open("/reader", 0);
    sem_t *writer_sem = sem_open("/writer", 0);
    sem_t *mutex_sem = sem_open("/muter", 0);

    // GET ARGV VARIABLES AND CONVERT THEM TO THE CORRECT DATA TYPES (IF NEEDED)
    char *filename = argv[1];
    int recid = atoi(argv[2]);
    int value = atoi(argv[3]);
    int dw = atoi(argv[4]);
    int shmid = atoi(argv[5]);

    // ATTACH THE SHARED MEMORY TO THIS PROGRAM
    SharedData *data = (SharedData*) shmat(shmid, NULL, 0);  

    // OPEN THE FILE
    FILE *file = fopen(filename, "r");
    
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

    // CLOSE THE FILE
    fclose(file);
    // CLOSE THE SEMAPHORES
    sem_close(reader_sem);
    sem_close(writer_sem);
    sem_close(mutex_sem);

    // HANDLE SLEEPING
    int sec = rand() % (dw + 1);
    sleep(sec);


    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double total_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9; // CALCULATE TIME AND CONVERT TO SECONDS
    // WRITE THE ELAPSED TIME TO THE SHARED MEMORY AND INCREMENT THE COMPLETED READERS

    // DETACH THE SHARED MEMORY FROM THIS PROGRAM
    shmdt(data);
}