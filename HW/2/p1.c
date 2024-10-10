#include <stdio.h>
#include <time.h>

int main(){
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
}