#include <stdio.h>
#include <time.h>
#include <limits.h>

int main(){
    int j = 0;
    double average = 0;
    clock_t start, end;

    while(1){
        j++;
        start = clock();
        // Start of task
        long sum = 0;
        for(unsigned i = 0; i< UINT_MAX*3/4; i++){
            sum += i;
        }
        // End of task
        end = clock();

        average = average * (j-1)/j +  ((double) (end-start) / CLOCKS_PER_SEC)/j;

        printf("Instantaneous Time: %f, Average: %f\n", 
         (double) (end-start) / CLOCKS_PER_SEC,
         average
        );

    }
}