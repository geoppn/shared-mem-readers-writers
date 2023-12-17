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

    char *comma = strchr(argv[2], ','); // THIS CHECKS IF A COMMA IS PRESENT IN THE ARGUMENT
    int recid;
    int consecutive_recs = -1;
    if (comma) { // IF A COMMA IS PRESENT, THEN THE ARGUMENT IS A RANGE
        sscanf(argv[2], "%d,%d", &recid, &consecutive_recs); // SEPARETE THE ARGUMENT INTO TWO DIFFERENT VARIABLES
    } else { // IF A COMMA IS NOT PRESENT, THEN THE ARGUMENT IS A SINGLE RECORD ID
        recid = atoi(argv[2]);
    }

    int dr = atoi(argv[3]);
    int shmid = atoi(argv[4]);

    // ATTACH THE SHARED MEMORY TO THIS PROGRAM
    SharedData *data = (SharedData*) shmat(shmid, NULL, 0);  

    // OPEN THE FILE
    FILE *file = fopen(filename, "r");
    
    // CONSECUTIVE RECORDS EXIST:
    if (consecutive_recs != -1) {
        int total_bal=0;
        for (int i = 0; i < consecutive_recs; i++) { // ITERATE THROUGH THE CONSECUTIVE RECORDS INCLUDING THE FIRST ONE
            // SEEK TO THE RECORD
            fseek(file, (recid + i) * sizeof(Record), SEEK_SET);
            
            // READ THE FIRST RECORD
            Record record;
            fread(&record, sizeof(Record), 1, file);
            // INCREMENT RECORDS HANDLED IN SHARED MEMORY
            printf("Reader with PID: %d has read the record: ID: %d, Surname: %s, Name: %s, Balance: %d\n", getpid(), record.customerID, record.lName, record.fName, record.balance);
            total_bal+=record.balance;
        }
        double avg_bal = total_bal/consecutive_recs+1;
        printf("Average balance of the records read by reader with PID: %d: %f\n", getpid(), avg_bal);
    } else { // IF THERE ARE NO CONSECUTIVE RECORDS
        fseek(file, recid * sizeof(Record), SEEK_SET);

        Record record;
        fread(&record, sizeof(Record), 1, file);
        // INCREMENT RECORDS HANDLED IN SHARED MEMORY
    }

    // CLOSE THE FILE
    fclose(file);
    // CLOSE THE SEMAPHORES
    sem_close(reader_sem);
    sem_close(writer_sem);
    sem_close(mutex_sem);

    // HANDLE SLEEPING
    int sec = rand() % (dr + 1);
    sleep(sec);


    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double total_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9; // CALCULATE TIME AND CONVERT TO SECONDS
    // WRITE THE ELAPSED TIME TO THE SHARED MEMORY AND INCREMENT THE COMPLETED READERS

    // DETACH THE SHARED MEMORY FROM THIS PROGRAM
    shmdt(data);
}