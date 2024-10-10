
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

struct timeval tv;

#define MAX 1000
#define MAX_CHILD_THREADS 10
char location[MAX];
long long timestamp;
long buffer[1];
long long time_buffer[1];

pthread_t childThreads[MAX_CHILD_THREADS];
sem_t mySem;

long prevLoc=0, prevTS=0;
long nextLoc=0, nextTS = 0;
long buttonTS = 0;


void* Thread0(void*);
void* Child_Thread(void*);


int main(void){
	
	int pfd = open("/tmp/N_pipe1", O_RDONLY);

	pthread_t thread0;

	pthread_create(&thread0, NULL, &Thread0, NULL);
	
	while (1) {

		// Read from N_Pipe1 and store the values in buffer sizeof(char)
		if(read(pfd, buffer, sizeof(char)) > -1){
			//Get Locaton
			prevLoc = nextLoc;
			nextLoc = *((char *) buffer);
			printf("Location: %i, ", location[0]);

			//Get Time
			gettimeofday(&tv, NULL);

			prevTS = nextTS;
			nextTS = tv.tv_sec/1000.0+tv.tv_usec*1000.0;

			printf("Timestamp: %ld\n", nextTS);
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

			pthread_create(childThreads + idx, NULL, &Child_Thread, NULL);
			
			idx++;
			
			if(idx >= MAX_CHILD_THREADS){
				idx = 0;
			}
		}

	}

	pthread_exit(NULL);
}

/**	@brief
 * 	This function is going to interpolate the the position of the button press
 * 	event based on the previous and next GPS time and location information. It
 * 	is going to use the slope intercept form to interpolate the value.
 */
void* Child_Thread(void* ptr){


	nextLoc = (long)location[0];
	nextTS 	= (long)timestamp;


	// interpolate the location of the button press event based on the info
	// received through all the threads.
	long double deltaLoc 	= (long double)(nextLoc - prevLoc);
	long double deltaTS		= (long double)(nextTS - prevTS);
	long double slope 		= (long double)(deltaLoc/deltaTS);
	long double tmp 		= ((long double)slope)*((long double)(buttonTS - prevTS));
	long double y0 			= (long double)tmp + (long double)prevLoc;

	// Now print the information on the screen. We need to be careful in printing
	// the data. As we have 5 threads running concurrently and all of the them 
	// are trying to print on the screen at the same time, we need to restrict
	// the access so that while only one thread printing the info on the screen
	// the rest of the threads will wait for it to finish its printing job. We
	// need a semaphore for that.

	printf("\nInterpolation Results\n");
	printf("\n---------------------\n");
	printf("GPS Previous TS = %lf \t GPS Previous Location = %lf \n", \
			(long double)prevTS, (long double)prevLoc);
	printf("Button TS = %lf \t Button Location = %lf\n", \
			(long double)buttonTS, (long double)y0);
	printf("GPS Next TS = %lf \t GPS Next Location = %lf\n", \
			(long double)nextTS, (long double)nextLoc);
	printf("\n\n");


	pthread_exit(NULL);
}
