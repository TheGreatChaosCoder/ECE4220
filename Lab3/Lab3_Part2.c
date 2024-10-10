#include <errno.h>
#include <semaphore.h>
#include <signal.h>   
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <wiringPi.h>
#include <pthread.h>

const int GREEN_LED_GPIO = 4;
const int RED_LED_GPIO = 2;
const int YELLOW_LED_GPIO = 3;
const int BLUE_LED_GPIO = 5;
const int WALK_BUTTON_GPIO = 16;

sem_t lights_mutex; 
int lights[3]; //green, yellow, walk

void setLightsOn()
{
    digitalWrite(GREEN_LED_GPIO,  lights[0] == 1 ? HIGH : LOW);
    digitalWrite(YELLOW_LED_GPIO, lights[1] == 1 ? HIGH : LOW);
    digitalWrite(RED_LED_GPIO,    lights[2] == 1 ? HIGH : LOW);
}

void * greenSideLight(void * ptr){
    while(1)
    {
        printf("Green\n");
        sem_wait(&lights_mutex);
        lights[0] = 1;
        lights[1] = 0;
        lights[2] = 0;
        setLightsOn();
        sleep(1);
        sem_post(&lights_mutex);
    }

    pthread_exit(NULL);
}

void * yellowSideLight(void * ptr){
    while(1)
    {
        printf("Yellow\n");
        sem_wait(&lights_mutex);
        lights[0] = 0;
        lights[1] = 1;
        lights[2] = 0;
        setLightsOn();
        sleep(1);
        sem_post(&lights_mutex);
    }

    pthread_exit(NULL);
}

void * walkLight(void * ptr){
    while(1)
    {
        if(digitalRead(WALK_BUTTON_GPIO) == HIGH){
            printf("Red\n");
            sem_wait(&lights_mutex);
            lights[0] = 0;
            lights[1] = 0;
            lights[2] = 1;
            setLightsOn();
            sleep(2);
            sem_post(&lights_mutex);
        }
    }

    pthread_exit(NULL);
}


int main(void)
{
    int PTL1 = 15;
    int PTL2 = 10;
    int PPL = 10;

    // Set up all GPIOs
    wiringPiSetupGpio();

    pinMode(RED_LED_GPIO, OUTPUT);
    pinMode(GREEN_LED_GPIO, OUTPUT);
    pinMode(YELLOW_LED_GPIO, OUTPUT);
    pinMode(BLUE_LED_GPIO, OUTPUT);

    pinMode(WALK_BUTTON_GPIO, INPUT);
    pullUpDnControl(WALK_BUTTON_GPIO, PUD_DOWN);

    // Force Blue LED Off
    digitalWrite(BLUE_LED_GPIO, LOW);

    // Priority Scheudling
    pthread_t gThrd, yThrd, wThrd;
    pthread_attr_t gAttr, yAttr, wAttr;
    struct sched_param gParam, yParam, wParam;

    // Initialize Semaphore
    sem_init(&lights_mutex, 0, 1); 
   
    // Initialize Thread Attributes;
    pthread_attr_init(&gAttr);
    pthread_attr_init(&yAttr);
    pthread_attr_init(&wAttr);

    // Set Scheduling Policy
    pthread_attr_setschedpolicy(&gAttr, SCHED_FIFO);
    pthread_attr_setschedpolicy(&yAttr, SCHED_FIFO);
    pthread_attr_setschedpolicy(&wAttr, SCHED_FIFO);

    // Set Priority
    gParam.sched_priority = 20;
    yParam.sched_priority = 20;
    wParam.sched_priority = 20;

    pthread_attr_setschedparam(&gAttr, &gParam);
    pthread_attr_setschedparam(&yAttr, &yParam);
    pthread_attr_setschedparam(&wAttr, &wParam);

    // Create Threads
    pthread_create(&gThrd, &gAttr, &greenSideLight, NULL);
    pthread_create(&yThrd, &yAttr, &yellowSideLight, NULL);
    pthread_create(&wThrd, &wAttr, &walkLight, NULL);

    // Join Threads
    pthread_join(gThrd, NULL);
    pthread_join(yThrd, NULL);
    pthread_join(wThrd, NULL);

    // Destroy Attributes
    pthread_attr_destroy(&gAttr);
    pthread_attr_destroy(&yAttr);
    pthread_attr_destroy(&wAttr);

    return 0;
}
