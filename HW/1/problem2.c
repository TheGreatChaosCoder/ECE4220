#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <limits.h>

void child(void) {
    while (1) {
        // Start of task
        int product = 0;
        for(unsigned i = 0; i< UINT_MAX/2; i++){
            product *= i;
        }
        // End of task
        
        printf("World\n");
        fflush(stdout); 
    }
 }
int main(void) {
    pid_t pid ;

    if ((pid = fork()) < 0) {
        printf("fork error\n");
        exit(-1); 
    }
    if (pid == 0) { // child process
        child();
    }

    while (1) {
        // Start of task
        long sum = 0;
        for(unsigned i = 0; i< UINT_MAX*3/4; i++){
            sum += i;
        }
        // End of task
        printf("Hello ");
        fflush(stdout); 
    }

    return(0); 
}