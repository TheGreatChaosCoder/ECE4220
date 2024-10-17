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
#include <string.h>
#include <errno.h>
#include <wiringPi.h>

const int BUTTON_GPIO = 16;

struct period_info {
    struct timespec next_period;  // sec, nano-sec
    long period_ns;
};

// Thread related Real-time functions
static void periodic_task_init(struct period_info *pinfo, long period);
static void increment_period(struct period_info *pinfo);
static void wait_rest_of_period(struct period_info *pinfo);


int main(void)
{
    int pfd;
    struct timeval tv;
    struct period_info time_info;
    long time_buffer[1];

    // Set up pipe
    if( -1 == mkfifo("/tmp/N_pipe2", 0666)){
        printf("N_pipe2 has probably been made\n");
    }

    pfd = open("/tmp/N_pipe2", O_WRONLY);

    if(pfd == -1){
        printf("Cannot write to N_pipe2, exiting\n");
        return 1;
    }

    // Set up GPIOs
    printf("Setting up GPIO\n");
    wiringPiSetupGpio();

    pinMode(BUTTON_GPIO, INPUT);
    pullUpDnControl(BUTTON_GPIO, PUD_DOWN);

    // Initialize the perods
	periodic_task_init(&time_info, 60 * 1000000.0);

    printf("Starting Loop\n");
    while(1)
	{
        if(digitalRead(BUTTON_GPIO) == HIGH){
			gettimeofday(&tv, NULL);
            time_buffer[0] = tv.tv_sec*1000.0+tv.tv_usec/1000.0;
            printf("Button Pressed\n");

            write(pfd, time_buffer, sizeof(long));
        }

		wait_rest_of_period(&time_info);
	}

    return 0;
}

// From Lab 2

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