
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

struct timeval tv;

char location[MAX];
long long timestamp;
long buffer[1];
long long time_buffer[1];
int displayFd[2];

pthread_t childThreads[MAX_CHILD_THREADS];
sem_t locationTimeSem;

long prevLoc=0, nextLoc=0;
long prevTS=0, nextTS = 0;
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
	
	while (1) {

		// Read from N_Pipe1 and store the values in buffer sizeof(char)
		if(read(pfd, location, sizeof(char)) > -1){
			//Get Time
			gettimeofday(&tv, NULL);

			while (tv.tv_usec >= 1000000) {
				/* timespec sec overflow */
				tv.tv_sec++;
				tv.tv_usec -= 1000000;
			}

			// Set all variables
			sem_wait(&locationTimeSem);
			prevLoc = nextLoc;
			nextLoc = *((char *) location);
			prevTS = timestamp;
			timestamp = tv.tv_sec*1000+tv.tv_usec/1000.0;
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
		if(read(pfd, time_buffer, sizeof(long)) > -1){
			buttonTS = *((long *) time_buffer);
			printf("Button Timestamp\n: %ld\n", buttonTS);

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

	nextLoc = (long) location[0];
	nextTS 	= timestamp;


	// interpolate the location of the button press event based on the info
	// received through all the threads.
	sem_wait(&locationTimeSem);
	long double deltaLoc 	= (long double)(nextLoc - prevLoc);
	long double deltaTS		= (long double)(nextTS - prevTS);
	long double slope 		= (long double)(deltaLoc/deltaTS);
	long double tmp 		= ((long double)slope) *((long double)(buttonTS - prevTS));
	long double y0 			= (long double)tmp + (long double)prevLoc;
	sem_post(&locationTimeSem);

	// Fill in data structure to send through pipe
	results.nextLoc = nextLoc;
	results.prevLoc = prevLoc;
	results.nextTS = nextTS;
	results.prevTS = prevTS;
	results.buttonTS = buttonTS;
	results.y0 = y0;

	// Send through pipe
	write(displayFd[1], &results, sizeof(results));

	pthread_exit(NULL);
	return 0;
}
