#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <wiringPi.h>

const int GREEN_LED_GPIO = 4;
const int RED_LED_GPIO = 2;
const int YELLOW_LED_GPIO = 3;
const int WALK_BUTTON_GPIO = 16;

const int PTL1 = 1;
const int PTL2 = 1;
const int PPL = 1;

sem_t lights_mutex; 

void * greenSideLight(void * ptr){

}

void * yellowSideLight(void * ptr){
    
}

void * walkLight(void * ptr){
    
}


int main(void)
{
    // Set up all GPIOs
    wiringPiSetupGpio();

    pinMode(RED_LED_GPIO, OUTPUT);
    pinMode(GREEN_LED_GPIO, OUTPUT);
    pinMode(YELLOW_LED_GPIO, OUTPUT);

    pinMode(WALK_BUTTON_GPIO, INPUT);
    pullUpDnControl(WALK_BUTTON_GPIO, PUD_DOWN);

    // Priority Scheudling
    int lights[3] = [1,0,0]; //green, yellow, walk
    pthread_t gThrd, yThrd, wThrd;
    pthread_attr_t gAttr, yAttr, wAttr;

    // Initialize Semaphore
    sem_init(&lights_mutex, 0, 1); 

    // Initialize Thread Attributes;
    pthread_attr_init(&gAttr);
    pthread_attr_init(&yAttr);
    pthread_attr_init(&wAttr);

    // Set Thread Priorities
    pthread_attr_getschedpolicy(&gAttr, &PTL1);
    pthread_attr_getschedpolicy(&yAttr, &PTL2);
    pthread_attr_getschedpolicy(&wAttr, &PPL);

    // Create Threads

    // Join Threads

    // Destroy Attributes
    pthread_attr_destroy(&gAttr);
    pthread_attr_destroy(&yAttr);
    pthread_attr_destroy(&wAttr);

    return 0;
}
