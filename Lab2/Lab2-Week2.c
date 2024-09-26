#include <fcntl.h>
#include <getopt.h>
#include <linux/types.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#define MY_PRIORITY (49)  // kernel is priority 50


void print_results();


//Define global variables
char fullString[20][256];
const long taskPeriod_ns = 1000000000; 

struct Buffers {
    char *sBuffer1;        
    char *sBuffer2;
};

// Timer Functions
// ===============
// Timer related structures and functions:
struct period_info {
    struct timespec next_period;        // sec, nano-sec
    long period_ns;
};


// Thread related Real-time functions
// ========================

static void periodic_task_init(struct period_info *pinfo, long period);
static void increment_period(struct period_info *pinfo);
static void wait_rest_of_period(struct period_info *pinfo);


// Thread-1 to read from "first.txt"

void *getFirstThd(void *ptr){
	printf("Starting thread 1...\n");

	//Get string pointer from main
	char *commonBuff;
	commonBuff = (char *)ptr;

	// Declare it as a real time task and pass the necessary params to the scheduler 
	struct sched_param param;
    param.sched_priority = MY_PRIORITY;
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        printf("Run the program as a sudo user\n");	//Get string pointer from main
		perror("sched_setscheduler failed, thread 2");
    	exit(20);
    }
	

	printf("Scheduled thread 1...\n");

        
    //Open File
	FILE * file = fopen("first.txt", "r");
	
	//Check to make sure file opened correctly
	if (file == NULL){
        printf("Cannot open file first.txt, thread 1");
		exit(20);
    }
        

	// Initialize the periodic Task and read line at a time from "First.txt"
	struct period_info time_info;
	periodic_task_init(&time_info, taskPeriod_ns);
	
	//Loop{
	//Read a line then wait_rest_of_period
	//}
	while(fgets(commonBuff, sizeof(commonBuff)*256, file))
	{
		wait_rest_of_period(&time_info);
	}

	//Exit pthread
	fclose(file);
	pthread_exit(NULL);
	
}

// Thread-2 to read from "second.txt"
void *getSecThd(void *ptr)
{
	printf("Starting thread 2...\n");

	//Get string pointer from main
	char *commonBuff;
	commonBuff = (char *)ptr;


	// Declare it as a real time task and pass the necessary params to the scheduler 
	struct sched_param param;
    param.sched_priority = MY_PRIORITY;
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        printf("Run the program as a sudo user\n");
 	    perror("sched_setscheduler failed, thread 2");
    	exit(20);
    }

	printf("Scheduled thread 2...\n");

        
    //Open File
	FILE * file = fopen("second.txt", "r");
	
	//Check to make sure file opened correctly
	if (file == NULL){
        printf("Cannot open file second.txt, thread 2");
		exit(20);
    }
        

	// Initialize the periodic Task
	struct period_info time_info;
	periodic_task_init(&time_info, taskPeriod_ns);
	
	//Loop{
	//Read a line then wait_rest_of_period
	//}
	while(fgets(commonBuff, sizeof(commonBuff)*256, file))
	{
		wait_rest_of_period(&time_info);
	}

	fclose(file);

	//Exit pthread
	pthread_exit(NULL);
}

// Thread-3 to copy the results into final buffer.
void *getThirdThd(void *ptr)
{
	printf("Starting thread 3...\n");

	struct Buffers * buffers = (struct Buffers *) (ptr);
	int done1 = 0;
	int done2 = 0;
	int lineCount = 0;

	// Declare it as a real time task and pass the necessary params to the scheduler 
	struct sched_param param;
    param.sched_priority = MY_PRIORITY;
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        printf("Run the program as a sudo user\n");
 	    perror("sched_setscheduler failed, thread 3");
    	exit(20);
    }

	printf("Scheduled thread 3...\n");

	struct period_info time_info;
	periodic_task_init(&time_info, taskPeriod_ns);

	while(lineCount < 20 || (done1 && done2)){
		done1 = (buffers->sBuffer1 == "Done");
		if(!done1){
			strcpy(fullString[lineCount], buffers->sBuffer1);
			//printf("Thread 3: Line %i -> %s\n", lineCount+1, fullString[lineCount]);
			lineCount++;
		}

		done2 = (buffers->sBuffer2 == "Done");
		if(!done2){
			strcpy(fullString[lineCount], buffers->sBuffer2);
			//printf("Thread 3: Line %i -> %s\n", lineCount+1, fullString[lineCount]);
			lineCount++;
		}

		wait_rest_of_period(&time_info);
	}

	pthread_exit(NULL);

}

int main(void) 
{
	//Declare variables	
	pthread_t thrd1, thrd2, thrd3;

        
    char sBuffer1[256];
	char sBuffer2[256];
	struct Buffers myBuffers;
	myBuffers.sBuffer1 = (char *)&sBuffer1;
	myBuffers.sBuffer2 = (char *)&sBuffer2;



	// Create 3 different threads -- First 2 threads will read from two
	// separate files and the 3rd will merge the two sets of information into one.
	printf("Creating thread 1...\n");
	pthread_create(&thrd1, NULL, &getFirstThd, (void *) sBuffer1);
	printf("Creating thread 2...\n");
	pthread_create(&thrd2, NULL, &getSecThd,   (void *) sBuffer2);
	printf("Creating thread 3...\n");
	pthread_create(&thrd3, NULL, &getThirdThd, (void *) &myBuffers);


	//Join pthreads and check to make sure they joined correctly
	if(pthread_join(thrd1, NULL) != 0){
		printf("Failed to join thread 1");
		exit(20);
	}
	if(pthread_join(thrd2, NULL) != 0){
		printf("Failed to join thread 2");
		exit(20);
	}
	if(pthread_join(thrd3, NULL) != 0){
		printf("Failed to join thread 3");
		exit(20);
	}

	print_results();
	return 0;
}


// Helper Functions
// ================

// Function to print out results
void print_results(){
	printf("Starting print_results():\n");
	for(int i =0; i<20; i++){
		puts(fullString[i]);
	}
}

// Assume nanosecond precision for period

//Write a function to determine the starting time of the thread
static void periodic_task_init(struct period_info *pinfo, long period)
{
	pinfo->period_ns = period;
 
    clock_gettime(CLOCK_MONOTONIC, &(pinfo->next_period));
}

// Write a function to the determine the ending time of the thread based on the initialized time
static void increment_period(struct period_info *pinfo)
{
	pinfo->next_period.tv_nsec += pinfo->period_ns;
 
    while (pinfo->next_period.tv_nsec >= 1000000000) {
        /* timespec nsec overflow */
        pinfo->next_period.tv_sec++;
        pinfo->next_period.tv_nsec -= 1000000000;
    }
}


// Write a function to sleep for the remaining time of the period after finishing the task
static void wait_rest_of_period(struct period_info *pinfo)
{
	increment_period(pinfo);
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &pinfo->next_period, NULL);
}





