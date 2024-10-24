
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <stdint.h>
#include <math.h>
#include <semaphore.h>
#include <sys/timerfd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define MAX 1000
#define MAX_CHILD_THREADS 15

typedef struct{
	long buttonTS;
	long prevLoc, nextLoc;
	long prevTS, nextTS;
	long double y0;
} InterpResults;

struct timespec tv;

char location[MAX];
long long timestamp;
long buffer[1];
long long time_buffer[1];
int displayFd[2];

pthread_t childThreads[MAX_CHILD_THREADS];
sem_t locationTimeSem, buttonTSSem;

long prevLoc=0, nextLoc=0;
long long prevTS=0, nextTS = 0;
long buttonTS = 0;


void* Thread0(void*);
void* Thread1(void*);
void* Child_Thread(void*);


int main(void){
	
	// Create pipe for display
	if(-1 == pipe(displayFd)){
		printf("Display pipe is not created, exiting...\n");
		return 1;
	}

	// Read from Named Pipe
	int pfd = open("/tmp/N_pipe1", O_RDONLY);

	// Set up threads
	pthread_t thread0, thread1;

	pthread_create(&thread0, NULL, &Thread0, NULL);
	pthread_create(&thread1, NULL, &Thread1, NULL);

	// Set up semaphores
    sem_init(&locationTimeSem, 0, 1); 
	sem_init(&buttonTSSem, 0, 1); 
	
	while (1) {

		// Read from N_Pipe1 and store the values in buffer sizeof(char)
		if(read(pfd, location, sizeof(char)) > -1){
			sem_wait(&locationTimeSem);
			//Get Time
			clock_gettime(CLOCK_MONOTONIC, &tv);

			// Set all variables
			prevLoc = nextLoc;
			nextLoc = *((char *) location);
			prevTS = timestamp;
			timestamp = tv.tv_sec*1000.0+tv.tv_nsec/1000000.0;
			sem_post(&locationTimeSem);
		}
		
	}

	// Join thread -- won't reach here anyways!
	pthread_join(thread0, NULL);


	pthread_exit(NULL);
	return 0;
}

void* Thread0(void* ptr) {

	int pfd = open("/tmp/N_pipe2", O_RDONLY);
	int idx = 0;

	while(1){
		
		// Implement a ring "buffer" to generate new threads
		if(read(pfd, time_buffer, sizeof(long long)) > -1){

			// Hold semaphore here, release it in the thread
			sem_wait(&buttonTSSem);
			buttonTS = *(time_buffer);

			pthread_create(childThreads + idx, NULL, &Child_Thread, NULL);
			
			idx++;
			
			if(idx >= MAX_CHILD_THREADS){
				idx = 0;
			}
		}

	}

	pthread_exit(NULL);
}

void* Thread1(void* ptr){
	InterpResults results;

	while(1){
		if(read(displayFd[0], &results, sizeof(results)) > -1){
			printf("\nInterpolation Results\n");
			printf("\n---------------------\n");
			printf("GPS Previous TS = %lf \t GPS Previous Location = %lf\n", \
				(long double)results.prevTS, (long double)results.prevLoc);
			printf("Button TS = %lf \t Button Location = %lf\n", \
				(long double)results.buttonTS, (long double)results.y0);
			printf("GPS Next TS = %lf \t GPS Next Location = %lf\n", \
				(long double)results.nextTS, (long double)results.nextLoc);
			printf("\n\n");
		}

	}

	pthread_exit(NULL);
	return 0;
}

/**	@brief
 * 	This function is going to interpolate the the position of the button press
 * 	event based on the previous and next GPS time and location information. It
 * 	is going to use the slope intercept form to interpolate the value.
 */
void* Child_Thread(void* ptr){

	InterpResults results;
	long long buttonTS_tmp;

	// Hold in parent thread, release semaphore here
	buttonTS_tmp = buttonTS;
	sem_post(&buttonTSSem);

	// interpolate the location of the button press event based on the info
	// received through all the threads.
	sem_wait(&locationTimeSem);

	nextLoc = (long) location[0];
	nextTS 	= timestamp;
	long double deltaLoc 	= (long double)(nextLoc - prevLoc);
	long double deltaTS		= (long double)(nextTS - prevTS);
	long double slope 		= (long double)(deltaLoc/deltaTS);
	long double tmp 		= ((long double)slope) *((long double)(buttonTS_tmp - prevTS));
	long double y0 			= (long double)tmp + (long double)prevLoc;

	// Fill in data structure to send through pipe
	results.nextLoc = nextLoc;
	results.prevLoc = prevLoc;
	results.nextTS = nextTS;
	results.prevTS = prevTS;
	results.buttonTS = buttonTS_tmp;
	results.y0 = y0;

	sem_post(&locationTimeSem);

	// Send through pipe
	write(displayFd[1], &results, sizeof(results));

	pthread_exit(NULL);
	return 0;
}
