#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/timerfd.h>
#include <inttypes.h>
#include <time.h>
#include <semaphore.h>
#include <errno.h>


//Declare Variables to store the given matrix and final results

//Declare structure to hold the convolution info (Like number of rows/cols)
typedef struct MatrixConvInfo {
    int rows;
    int cols;
    int filtered_rows;
    int filtered_cols;
    int * filter;
    int * matrix;
} MatrixConvInfo;

typedef struct MatrixConvRowInfo {
    MatrixConvInfo * matrix;
    int row;
} MatrixConvRowInfo;

typedef struct MatrixConvElementInfo {
    MatrixConvInfo * matrix;
    int row;
    int col;
} MatrixConvElementInfo;

int ConvolutionOnElement(MatrixConvInfo * m, const int row, const int col){
    int i, j, index;
    int sum = 0;
    for(i=-m->filtered_rows/2; i<=m->filtered_rows/2; i++){
        for(j=-m->filtered_cols/2; j<=m->filtered_cols/2; j++){
            
            if((row+i)>=0 && (row+i)<m->rows && (col+j)>=0 && (col+j)<m->cols){
                index = (i+m->filtered_rows/2)*m->filtered_cols + j+m->filtered_cols/2;
                sum += (*(m->filter + index)) * (*(m->matrix + (row+i)*m->cols + col + j));
            }

        }
    }

    return sum;
}

void * ConvolutionPerElement(void * info){
    MatrixConvElementInfo * m = (MatrixConvElementInfo *) (info);
    int * filtered_element = malloc(sizeof(int));

    *(filtered_element) = ConvolutionOnElement(m->matrix, m->row, m->col);

    pthread_exit(filtered_element);
}

// 1. Single thread to evaluate the convolution 
void * ConvolutionPerMatrix(void *info){
    MatrixConvInfo * matrix = (MatrixConvInfo *) info;
    int * filtered_matrix = malloc(sizeof(int) * matrix->rows * matrix->cols);
    unsigned i,j;

    for(i=0; i<matrix->rows; i++){
        for(j=0; j<matrix->cols; j++){
            *(filtered_matrix + i*matrix->cols + j) = ConvolutionOnElement(matrix, i, j);
        }
    }

	pthread_exit(filtered_matrix);
}


// 2. Thread function to process one row of the given matrix
void * ConvolutionPerRow(void *info){
    MatrixConvRowInfo * m = (MatrixConvRowInfo *) (info);
    int * filtered_row = malloc(sizeof(int) * m->matrix->cols);
    unsigned i;

    for(i=0; i<m->matrix->cols; i++){
        filtered_row[i] = ConvolutionOnElement(m->matrix, m->row, i);
    }

    pthread_exit(filtered_row);
}

// 3. Thread to process each element of the matrix.

// Read File Function
MatrixConvInfo * readFile(char * fileName){
    MatrixConvInfo * info = malloc(sizeof(MatrixConvInfo));
    unsigned i,j;

    if (info == NULL){
        printf("Problem with generating matrixConvInfo object... returning NULL\n");
        return NULL;
    }

    FILE * file = fopen(fileName, "r");

    if (file == NULL){
        printf("Cannot open file %s, returning NULL\n", fileName);
    }

    fscanf(file, "%d", &(info->rows));
    fscanf(file, "%d", &(info->cols));

    info->matrix = malloc(sizeof(int) * info->rows * info->cols);

    if(info->matrix == NULL){
        printf("Cannot initialize matrix, returning NULL\n");
    }

    // Scan in matrix
    for(i=0; i<info->rows; i++){
        for(j=0; j<info->cols; j++){
            fscanf(file, "%d", info->matrix + i*info->cols + j);
        }
    }

    fscanf(file, "%d", &(info->filtered_rows));
    fscanf(file, "%d", &(info->filtered_cols));

    info->filter = malloc(sizeof(int) * info->filtered_rows * info->filtered_cols);

    if(info->filter == NULL){
        printf("Cannot initialize filter, returning NULL\n");
    }

    // Scan in matrix
    for(i=0; i<info->filtered_rows; i++){
        for(j=0; j<info->filtered_cols; j++){
            fscanf(file, "%d", info->filter + i*info->filtered_cols + j);
        }
    }

    fclose(file);

    return info;
}

// Helper Functions
// ================
//Function to print out the result matrix
void print_matrix(int *arr, int numRows, int numCols){
    printf("\n");
    for(int i = 0; i < numRows; i++){
        for(int j = 0; j < numCols; j++){
            printf("%3d ", *(arr + i*numCols + j));
        }
        printf("\n");
    }
    printf("\n");
}

// Main function
int main(int argc, char *argv[]) {
    printf("Starting Week 1 Program...\n\n");

    // Define variables
    clock_t start, end;
    MatrixConvInfo *matrix;
    int * filtered_matrix;
    int i,j;

	// Step-1
    // ======
    // Read the file and load the data matrix and filter vector information
    matrix = readFile("2x5.txt");

    if(matrix == NULL){
        return 0;
    }

    // Print the original matrix
    print_matrix(matrix->matrix, matrix->rows, matrix->cols);
    
    // Print the filter matrix
    print_matrix(matrix->filter, matrix->filtered_rows, matrix->filtered_cols);


	// Step-2
    // ======
    // Convolve using a single thread.
    printf("\n\n**Case 1: Convolution using single thread**\n");

	// Start Timing
    start = clock();
    // Use pthread_create function to create pthreads
    pthread_t thread;
    pthread_create(&thread, NULL, &ConvolutionPerMatrix, matrix);
    // Use pthread_join the join the threads
    pthread_join(thread, (void **)&filtered_matrix);
	// End timing
    end = clock();

	// Print out search count with 1 thread and the time it took
    printf("Resultant filtered matrix:\n");
    print_matrix(filtered_matrix, matrix->rows, matrix->cols);
    printf("Time to compute on a single thread: %f sec\n", (double) (end-start) / CLOCKS_PER_SEC);

	// Step-3
    // ======
    // Convolution using 1 thread per row
    printf("\n\n**Case 2: Convolution using single thread per row**\n");
    
    // Start Timing
    start = clock();

    //Create threads 
    int * filtered_row[matrix->rows];
    pthread_t threads[matrix->rows];
    MatrixConvRowInfo rInfo[matrix->rows];

    for(i = 0; i<matrix->rows; i++){
        rInfo[i].matrix = matrix;
        rInfo[i].row = i;
        pthread_create(&threads[i], NULL, &ConvolutionPerRow, &rInfo[i]);
    }
    
    // Use pthread_join the join the threads
    for(i = 0; i<matrix->rows; i++){
        pthread_join(threads[i], (void **)&(filtered_row[i]));
        for(j = 0; j<matrix->cols; j++){
            *(filtered_matrix + i*matrix->cols + j) = filtered_row[i][j];
        }
    }

	// End timing
    end = clock();

	// Print out search count with 1 thread and the time it took
    printf("Resultant filtered matrix:\n");
    print_matrix(filtered_matrix, matrix->rows, matrix->cols);
    printf("Time to compute on a thread per row: %f sec\n", (double) (end-start) / CLOCKS_PER_SEC);

    // Step-4
    // ======
    // Convolution with 1 thread per element
    printf("\n\n**Case 3: Convolution with 1 thread per element**\n");

    // Start Timing
    start = clock();

    //Create threads 
    int * filtered_elements[matrix->rows][matrix->cols];
    pthread_t moreThreads[matrix->rows][matrix->cols];
    MatrixConvElementInfo eInfo[matrix->rows][matrix->cols];

    for(i = 0; i<matrix->rows; i++){
        for(j = 0; j<matrix->cols; j++){
            eInfo[i][j].matrix = matrix;
            eInfo[i][j].row = i;
            eInfo[i][j].col = j;
            pthread_create(&moreThreads[i][j], NULL, &ConvolutionPerElement, &eInfo[i][j]);
        }
    }
    
    // Use pthread_join the join the threads
    for(i = 0; i<matrix->rows; i++){
        for(j = 0; j<matrix->cols; j++){
            pthread_join(moreThreads[i][j], (void **)&(filtered_elements[i][j]));
            *(filtered_matrix + i*matrix->cols + j) = *(filtered_elements[i][j]);
        }
    }

	// End timing
    end = clock();

	// Print out search count with 1 thread and the time it took
    printf("Resultant filtered matrix:\n");
    print_matrix(filtered_matrix, matrix->rows, matrix->cols);
    printf("Time to compute on a thread per row: %f sec\n", (double) (end-start) / CLOCKS_PER_SEC);

    // Free Everything
    free(filtered_matrix);

    free(matrix->matrix);
    free(matrix->filter);
    free(matrix);

    for(i = 0; i<matrix->rows; i++){
        free(filtered_row[i]);
        for(j = 0; j<matrix->cols; j++){
            free(filtered_elements[i][j]);
        }
    }

    return 0;
}



