/*
*   CSE438 Embedded Systems Programming Spring 2022
*   Assignment 3: Accessing kernel hash tables via a device file interface
*/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define NUM_THREADS                         2

#define DEVICE_0_FILE                       "/dev/ht438_dev_0"
#define DEVICE_1_FILE                       "/dev/ht438_dev_1"

#define OUTPUT_FILE_1                       "t1_out"
#define OUTPUT_FILE_2                       "t2_out"

typedef struct ht_object {
    int key;
    char data[4]; 
} ht_object_t; 

struct filep {
    FILE *filep_in;
    FILE *filep_out;
};

char print_ret[4];
char *print_data(char *d)
{
    memset(print_ret, 0, sizeof(print_ret));

    for(int i=0; i<4; i++) {
        print_ret[i] = d[i];
    }

    return print_ret;
}

void data_copy(char *destination, char *source)
{
    for(int i=0; i<strlen(source); i++) {
        destination[i] = source[i];
    }
}

int fd0, fd1;

FILE *fin[2], *fout[2];//test_f1, *test_f2, *out_f1, *out_f2;

// Variable to get thread ID of main() thread
pthread_t main_thread_id;

//Declaration of thread IDs
pthread_t thread_id[NUM_THREADS];

//Thread attributes and Structure Parameters Declaration
pthread_attr_t thread_attr;
struct sched_param param;

// Barrier used to wait until all threads are 
// created so they can start at the same time
pthread_barrier_t barrier;

void *file_thread_func(void *args)
{
    int thread_number = *(int *)args;

    printf("Inside thread: %d\n", thread_number);

    char buffer[100];

    char operation = 0;
    char table_data[4] = {0};
    int table_num = 0;
    int table_key = 0;
    int sleep_time = 0;

    pthread_barrier_wait(&barrier);

    while(fgets(buffer, sizeof(buffer), fin[thread_number]))
    {
        // printf("%s\n", buffer);
        ht_object_t object = {0};

        if(buffer[0] == 'w') {
            memset(table_data, 0, 4);
            sscanf(buffer, "%c %d %d %4s", &operation, &table_num, &table_key, table_data);
            object.key = table_key;
            data_copy(object.data, table_data);
            printf("Operation: %c, Table: %d, Key: %d, Data: %s\n",operation, table_num, object.key, print_data(object.data));
            if(table_num == 0) {
                write(fd0, &object, sizeof(ht_object_t));
            }
            else if(table_num == 1) {
                write(fd1, &object, sizeof(ht_object_t));
            }
        }
        else if(buffer[0] == 'r') {
            char dummy_data[50] = {0};
            sscanf(buffer, "%c %d %d", &operation, &table_num, &table_key);
            object.key = table_key;
            if(table_num == 0) {
                read(fd0, &object, sizeof(ht_object_t));
            }
            else if(table_num == 1) {
                read(fd1, &object, sizeof(ht_object_t));
            }
            sprintf(dummy_data, "read from ht438_tbl_%d\nkey=%d\tdata=%s\n", table_num, object.key, print_data(object.data));
            fputs(dummy_data, fout[thread_number]);
            printf("Operation: %c, Table: %d, Key: %d, Data: %s\n",operation, table_num, object.key, print_data(object.data));
        }
        else if(buffer[0] == 's') {
            sscanf(buffer, "%c %d", &operation, &sleep_time);
            printf("Operation: %c, Sleep Time: %d us\n",operation, sleep_time);
            usleep(sleep_time);
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    int fret, ret, thread0 = 0, thread1 = 1;

    printf("argc = %d\n", argc);

    if(argc < 3) {
        printf("Invalid arguments! Please pass 2 test files as arguments\n");
        return -1;
    }

    printf("Opening File %s\n", DEVICE_0_FILE);
    fd0 = open(DEVICE_0_FILE, O_RDWR);
    if(fd0 < 0) {
        printf("Error opening %s\n", DEVICE_0_FILE);
        return -1;
    }
    printf("File Opened with fd: %d\n", fd0);

    printf("Opening file %s\n", DEVICE_1_FILE);
    fd1 = open(DEVICE_1_FILE, O_RDWR);
    if(fd1 < 0) {
        printf("Error opening %s\n", DEVICE_1_FILE);
        return -1;
    }
    printf("File Opened with fd: %d\n", fd1);

    fout[0] = fopen(OUTPUT_FILE_1, "w");
    if(fout[0] == NULL) {
        printf("Cannot open file %s\n", OUTPUT_FILE_1);
    }

    fout[1] = fopen(OUTPUT_FILE_2, "w");
    if(fout[1] == NULL) {
        printf("Cannot open file %s\n", OUTPUT_FILE_2);
    }

    fin[0] = fopen(argv[1], "r");
    if(fin[0] == NULL) {
        printf("Cannot open file %s\n", argv[1]);
    }

    fin[1] = fopen(argv[2], "r");
    if(fin[1] == NULL) {
        printf("Cannot open file %s\n", argv[2]);
    }

    // Changing the main thread scheduling policy and priority to highest
    main_thread_id = pthread_self();
    param.sched_priority = 90;
    pthread_setschedparam(main_thread_id, SCHED_FIFO, &param);

    //Initializing the thread attributes to Default
    pthread_attr_init(&thread_attr);

    //Setting Scheduling policy to (SCHED_FIFO) Priority Based Scheduling
    pthread_attr_setschedpolicy(&thread_attr, SCHED_FIFO);

    // As every thread has different priority, we use explicit sched params
    pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);

    pthread_barrier_init(&barrier, NULL, NUM_THREADS);

    printf("In main, creating %d threads\n", NUM_THREADS);

    param.sched_priority = 80;
    pthread_attr_setschedparam(&thread_attr, &param);
    pthread_create(&thread_id[0], &thread_attr, file_thread_func, &thread0);

    param.sched_priority = 70;
    pthread_attr_setschedparam(&thread_attr, &param);
    pthread_create(&thread_id[1], &thread_attr, file_thread_func, &thread1);

    for(int i=0; i<NUM_THREADS; i++)
    {
        pthread_join(thread_id[i], NULL);
        printf("Thread %d exited\n", i);
    }

    pthread_barrier_destroy(&barrier);

    pthread_attr_destroy(&thread_attr);

    printf("In main, out from threads\n");
    
    fret = fclose(fin[0]);
    if(fret) {
        printf("Cannot close file %s\n", argv[1]);
    }

    fret = fclose(fin[1]);
    if(fret) {
        printf("Cannot close file%s\n", argv[2]);
    }

    fret = fclose(fout[0]);
    if(fret) {
        printf("Cannot close file%s\n", OUTPUT_FILE_1);
    }

    fret = fclose(fout[1]);
    if(fret) {
        printf("Cannot close file%s\n", OUTPUT_FILE_2);
    }

    ret = close(fd0);
    if(ret) {
        printf("Error closing %s\n", DEVICE_0_FILE);
    }

    ret = close(fd1);
    if(ret) {
        printf("Error closing %s\n", DEVICE_1_FILE);
    }

    return 0;
}
