#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "SharedMemory.h"

#define MAX_VALUE 150 // MAXIMUM ABSOLUTE VALUE TO ADD/SUBTRACT FROM THE RECORDS BALANCE
#define MAX_NUM_RECORDS 5 // MAXIMUM NUMBER OF CONSECUTIVE RECORDS THAN CAN BE READ

int main(int argc, char *argv[]) {
    // DECLARE ARGV VARIABLES
    char *filename = NULL;
    int dw;
    int dr;
    // ITERATE ARGV VARIABLES IN A WAY THAT FLAGS CAN BE GIVEN IN ANY ORDER
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            filename = argv[i + 1];
        } else if (strcmp(argv[i], "-dw") == 0 && i + 1 < argc) {
            dw = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-dr") == 0 && i + 1 < argc) {
            dr = atoi(argv[i + 1]);
        } else {
            fprintf(stderr, "Invalid command line argument: %s\n", argv[i]);
            fprintf(stderr, "Correct usage flags: ./main -f filename -dw writer_delay -dr reader_delay \n");
            return 1;
        }
    }

    // CHECK IF THE FILENAME, DW AND DR ARE VALID!
    if (filename == NULL || dw <= 0 || dr <= 0) {
        fprintf(stderr, "Invalid command line arguments. Usage: ./main -f filename -dw delay_for_writers -dr delay_for_readers\n");
        return 1;
    }

    // OPEN THE FILE OR PROVIDE AN ERROR IF IT DOESNT EXIST
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "File not found: %s\n", filename);
        return 1;
    }
    // COUNT THE AMOUNT OF RECORDS FOR FURTHER USE
    int max_records = 0;
    char ch;
    while (!feof(file)) {
        ch = fgetc(file);
        if (ch == '\n') {
            max_records++;
        }
    }
    fclose(file);

    // CONVERT THE VALID DW AND DR INTO STRINGS SO THEY CAN BE PARSED TO THE CHILD PROCESSES LATER
    char dw_str[12];
    sprintf(dw_str, "%d", dw);
    char dr_str[12];
    sprintf(dr_str, "%d", dr);


    // CREATE KEY FOR SHARED MEMORY BASED ON SLIDES
    key_t key = ftok("main.c",65);

    // CREATE THE SHARED MEMORY
    int shmid = create_shared_memory(key);
    char shmid_str[12];
    sprintf(shmid_str, "%d", shmid);

    // INITIALIZE SEMAPHORES
    sem_t *mutex = create_semaphore("/mutex");
    sem_t *reader = create_semaphore("/reader");
    sem_t *writer = create_semaphore("/writer");

    srand(time(NULL)); // INITIALIZE RAND
    
    while (1) {
        // PRINT BASIC INFO
        printf("CONTROLS: 'r' to spawn a reader, 'w' to spawn a writer or 'q' to quit: ");
        char choice = getchar();

        if (choice == 'r') { // SPAWN A READER
            if (fork() == 0) {
                int flip = rand() % 2; // COIN FLIP TO SEE IF THE READER WILL READ ONE RECORD OR MULTIPLE
                int initial_recid = rand() % max_records; // INITIAL RECORD ID
                if (flip == 0) { // SINGLE RECORD ID
                    char recid_str[10];
                    sprintf(recid_str, "%d", initial_recid);
                    execlp("./reader", "./reader", filename, recid_str, dr_str, shmid_str, NULL);
                } else { // MULTIPLE RECORD IDs
                    int num_records = rand() % MAX_NUM_RECORDS + 1; // MAX_NUM_RECORDS = MAXIMUM NUMBER OF CONSECUTIVE RECORDS THAN CAN BE READ
                    char recid_str[20];
                    sprintf(recid_str, "%d,%d", initial_recid, initial_recid + num_records - 1);
                    execlp("./reader", "./reader", filename, recid_str, dr_str, shmid_str, NULL);
                }
            }
        } else if (choice == 'w') { // SPAWN A WRITER
            if (fork() == 0) {
                // GENERATE RANDOM VALUES 
                char recid[7];
                sprintf(recid, "%d", rand() % max_records); 
                char value[7];
                sprintf(value, "%d", rand() % (MAX_VALUE * 2) - MAX_VALUE);  // MAX VALUE = MAXIMUM ABSOLUTE VALUE TO ADD/SUBTRACT FROM THE RECORDS BALANCE

                execlp("./writer", "./writer", filename, recid, value, dw_str, shmid_str, NULL);
            }
        } else if (choice == 'q') {
            break;
        }

        // IGNORE THE NEWLINE CHARACTER
        getchar();
    }
    // CALCULATE ALL NECESSARY INFORMATION BEFORE PROGRAM EXIT
    SharedData* sharedData = (SharedData*) shmat(shmid, NULL, 0); // ACCESS THE SHARED MEMORY DATA
    // CALCUATE THE AVERAGE WRITER TIME
    double writer_total = 0;
    for (int i = 0; i < sharedData->completed_writers; i++) {
        writer_total += sharedData->writer_times[i];
    }
    double avg_writer_time = writer_total / sharedData->completed_writers;

    // CALCUATE THE AVERAGE READER TIME
    double reader_total = 0;
    for (int i = 0; i < sharedData->completed_readers; i++) {
        reader_total += sharedData->reader_times[i];
    }
    double avg_reader_time = reader_total / sharedData->completed_readers;

    // PRINT ALL THE INFORMATION
    printf("Number of completed readers: %d\n", sharedData->completed_readers);
    printf("Average reader activity time: %.2f seconds\n", avg_reader_time);
    printf("Number of completed writers: %d\n", sharedData->completed_writers);
    printf("Average writer activity time: %.2f seconds\n", avg_writer_time);
    printf("Maximum delay before starting work: %.2f seconds\n", sharedData->maxdelay);
    printf("Total number of processed records: %d\n", sharedData->processed_records);

    // CLEAN UP
    destroy_semaphore(mutex, "/mutex");
    destroy_semaphore(reader, "/reader");
    destroy_semaphore(writer, "/writer");

    destroy_shared_memory(shmid);
    
    return 0;
}