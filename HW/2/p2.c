#include <stdio.h>
#include <time.h>
#include <pthread.h>

void * thing(void * info){
    // Schedule it to be real time
    struct sched_param param;
    param.sched_priority = 49;
    sched_setscheduler(0, SCHED_FIFO, &param);

    int i = 0;
    double average = 0;
    clock_t start, end;

    while(1){
        i++;
        sleep(2);

        start = clock();
        printf("Iteration %i\n", i);
        end = clock();

        average = average * (i-1)/i +  ((double) (end-start) / CLOCKS_PER_SEC)/i;

        printf("Instantaneous Time: %f, Average: %f\n", 
         (double) (end-start) / CLOCKS_PER_SEC,
         average
        );

    }

    pthread_exit(NULL);
}

int main(){
    pthread_t thread;
    pthread_create(&thread, NULL, &thing, NULL);
    pthread_join(thread, NULL);
}