#include <stdio.h>
#include <pthread.h>
#include <limits.h>

void My_Thread( void *ptr ) {
    char *message;
    message = (char *) ptr;
    while (1) {
        // Start of task
        int product = 0;
        for(unsigned i = 0; i< UINT_MAX/2; i++){
            product *= i;
        }
        // End of task
        printf("%s", message);
        fflush(stdout); 
    }
    pthread_exit(0); 
}
int main(void) {
    struct timespec delay;
    pthread_t thread1, thread2;
    char *message1 = "Goodbye ";
    char *message2 = "Universe\n";

    pthread_create( &thread1, NULL,
                    (void*)&My_Thread, 
                    (void*) message2);

    while (1) {
        // Start of task
        long sum = 0;
        for(unsigned i = 0; i< UINT_MAX*3/4; i++){
            sum += i;
        }
        // End of task
        printf("%s", message1);
        fflush(stdout); 
    }
    return(0); 
}