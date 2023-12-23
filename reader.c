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
    int procrecords=0;
    struct timespec delay_start;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    clock_gettime(CLOCK_MONOTONIC, &delay_start);

    // OPEN THE SEMAPHORES WITHIN THIS PROGRAM
    sem_t *reader_sems[NUM_BLOCKS];
    sem_t *writer_sems[NUM_BLOCKS];
    sem_t *mutex;
    char sem_name[20];

    for (int i = 0; i < NUM_BLOCKS; i++) {
        sprintf(sem_name, "/reader_sem_%d", i);
        reader_sems[i] = sem_open(sem_name, 0);
        sprintf(sem_name, "/writer_sem_%d", i);
        writer_sems[i] = sem_open(sem_name, 0);
    }
    mutex = sem_open("/mutex", 0);

    // GET ARGV VARIABLES AND CONVERT THEM TO THE CORRECT DATA TYPES (IF NEEDED)
    char *filename = argv[1];
    char *comma = strchr(argv[2], ',');
    int recid;
    int consecutive_recs = -1;
    if (comma) {
        sscanf(argv[2], "%d,%d", &recid, &consecutive_recs);
    } else {
        recid = atoi(argv[2]);
    }
    int dr = atoi(argv[3]);

    int shmid = atoi(argv[4]);

    // ATTACH THE SHARED MEMORY TO THIS PROGRAM
    SharedData *data = (SharedData*) shmat(shmid, NULL, 0);  

    
    // OPEN THE FILE
    FILE *file = fopen(filename, "r");
    fseek(file, 0, SEEK_END); 
    long filesize = ftell(file); 
    int max_records = filesize / sizeof(Record); 
    int block_index;

    struct timespec delay_end;
    // CONSECUTIVE RECORDS EXIST:
    if (consecutive_recs != -1) {
        int total_bal=0;
        for (int i = 0; i < consecutive_recs; i++) { // ITERATE THROUGH THE CONSECUTIVE RECORDS INCLUDING THE FIRST ONE
            block_index = (recid-1 + i) / (max_records/NUM_BLOCKS); // CALCULATE WHICH BLOCK EACH OF THE RECORDS BELONGS TO
            printf("\033[1;33m");
            printf("Reader process is waiting for RECORD with id %d in block %d\n",recid + i, block_index);
            printf("\033[0m");
            start_read(reader_sems[block_index],writer_sems[block_index],data,block_index);
            printf("\033[1;33m");
            printf("DONE WAITING\n");
            printf("\033[0m");
            clock_gettime(CLOCK_MONOTONIC, &delay_end); // END THE TIMER WHEN THE PROCESS ENTERS ITS CS

            // SEEK TO THE RECORD
            fseek(file, ((recid - 1) + i) * sizeof(Record), SEEK_SET);
            procrecords++;
            
            // READ THE RECORD
            Record record;
            fread(&record, sizeof(Record), 1, file);

            printf("Reader with PID: %d has read the record: ID: %d, Surname: %s, Name: %s, Balance: %d\n", getpid(), record.customerID, record.lName, record.fName, record.balance);
            total_bal+=record.balance;

            if(i!=consecutive_recs-1){
                //sem_wait(mutex);
                end_read(reader_sems[block_index],writer_sems[block_index], data,block_index);
                //sem_post(mutex);
            }
        }
        double avg_bal = total_bal/consecutive_recs+1;
        printf("Average balance of the records read by reader with PID %d: %f\n", getpid(), avg_bal);
    } else { // IF THERE ARE NO CONSECUTIVE RECORDS
        block_index = (recid-1) / (double)(max_records/NUM_BLOCKS); 
        //sem_wait(mutex);
        start_read(reader_sems[block_index], writer_sems[block_index], data,block_index);
        //sem_post(mutex);
        clock_gettime(CLOCK_MONOTONIC, &delay_end); 
        fseek(file, (recid-1) * sizeof(Record), SEEK_SET);
        procrecords++;
        Record record;
        fread(&record, sizeof(Record), 1, file);
        
        printf("Reader with PID: %d has read the record: ID: %d, Surname: %s, Name: %s, Balance: %d\n", getpid(), record.customerID, record.lName, record.fName, record.balance);
        // INCREMENT RECORDS HANDLED IN SHARED MEMORY
    }

    // HANDLE SLEEPING
    int sec = rand() % (dr + 1);
    sleep(sec);

    //sem_wait(mutex);
    end_read(reader_sems[block_index], writer_sems[block_index], data,block_index);
    //sem_post(mutex);
    printf("\033[1;32m");
    printf("Reader posted for RECORD(s) %s\n",argv[2]);
    printf("\033[0m"); 

    // CLOSE THE FILE

    fclose(file);

    // CALCULATE THE TOTAL TIME AND DELAY
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double total_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9; // CALCULATE TIME AND CONVERT TO SECONDS
    double delay = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    // WRITE ALL INFO TO SHARED MEMORY
    sem_wait(mutex); // LOCK THE SHARED MEMORY

    for (int i = 0; i < NUM_BLOCKS; i++) { // ITERATE THROUGH THE READER TIMES ARRAY
        if (data->reader_times[i] == -1.0) { // IF THE CURRENT ELEMENT IS -1, THEN IT IS EMPTY
            data->reader_times[i] = total_time;
            break;
        }
    }
    data->completed_readers++; // INCREMENT THE COMPLETED READERS
    data->processed_records += procrecords; // INCREMENT THE PROCESSED RECORDS
    if (delay > data->maxdelay) { // UPDATE MAXDELAY IF VIABLE
        data->maxdelay = delay;
    }
    sem_post(mutex); // UNLOCK THE SHARED MEMORY    

    // CLOSE THE SEMAPHORES
    for (int i = 0; i < NUM_BLOCKS; i++) {
        sem_close(reader_sems[i]);
        sem_close(writer_sems[i]);
    }
    sem_close(mutex);

    // DETACH THE SHARED MEMORY FROM THIS PROGRAM
    shmdt(data);
}