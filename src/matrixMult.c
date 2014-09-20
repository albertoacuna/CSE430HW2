/*
 * The following is a simple program in C that takes the name of a file as a
 * command line argument and prints its contents into an output file. It uses
 * fscanf() and fprintf() to format the input and output respectively.
 *
 */
#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<pthread.h>
#include<string.h>

//prototypes
void matrixMultiply(void* params);
void matrixLoad(int rc, int**mat, FILE* fr1);
int matSize(char *line);
int rc;
int num_thrd;
pthread_mutex_t lock;

struct matrixMultParam{
    int** matrix1;
    int** matrix2;
    int** solution;
    int slice;
};

int main(int argc,char *argv[])
{
    double time_spent;
    clock_t begin, end;
    begin = clock();
    pthread_t* thread; //pointer to a group of threads
    num_thrd = atoi(argv[1]);
    
    //start timer
    FILE *fr1,*fr2,*fw; //File pointers
    int **mat1, **mat2, **solution; //To hold the matrix from the input file
    int rc1, rc2; //To hold the number of rows and columns
    int toStdOut=1; //This is a flag variable. 0 for file output,1 for std out
    char line[10000]; //for extract a char of lines
    
    //Check for the number of command line arguments
    if (argc!=5) {
        //Print output to standard output
        toStdOut=0;
    }
    
    //To read a file use the fopen() function to open it
    fr1=fopen(argv[2],"r");
    
    //If the file fails to open the fopen() returns a NULL
    if (fr1==NULL) {
        printf("Cannot open %s. Program terminated...",argv[1]);
        exit(1);
    }
    
    // Similar to the above method read the second file
    fr2=fopen(argv[3],"r");
    if (fr2==NULL) {
        printf("Cannot open %s. Program terminated...",argv[2]);
        exit(1);
    }
    
    //get matrice sizes and check if they are equal so they can be multiplied
    fgets(line,10000,fr1);
    rc1=matSize(line);
    fgets(line,10000,fr2);
    rc2 = matSize(line);
    
    if (rc1 == rc2)
    {
    	rc = rc1;
        //memory allocation for arrays
        mat1=(int **)malloc(sizeof(int *)*rc);
        mat2=(int **)malloc(sizeof(int *)*rc);
        solution=(int **)malloc(sizeof(int *)*rc);
        int i;
        for(i=0; i<rc1; ++i)
        {
            mat1[i] = malloc(rc1*sizeof(int));
        }
        
        for(i=0; i<rc1; ++i)
        {
            mat2[i] = malloc(rc1*sizeof(int));
        }
        
        for(i=0; i<rc1; ++i)
        {
            solution[i] = malloc(rc1*sizeof(int));
        }
        //-----------------------------------------------------------
        //load up both matricies
        matrixLoad(rc1, mat1, fr1);
        matrixLoad(rc1,mat2,fr2);

        pthread_mutex_init(&lock,NULL);

        struct matrixMultParam params;
        params.matrix1= mat1;
        params.matrix2= mat2;
        params.solution = solution;

        thread = (pthread_t*)malloc(num_thrd*sizeof(pthread_t));

        for(int i = 1; i < num_thrd; i++){
            pthread_mutex_lock(&lock);
            params.slice = i;
            if(pthread_create(&thread[i], NULL, matrixMultiply, (void*)&params) !=0){
                 printf("couldn't create thread");
                 free(thread);
                 exit(1);
            }
        }
        
            pthread_mutex_lock(&lock);
            params.slice = 0;
            matrixMultiply((void*)&params);
            pthread_mutex_destroy(&lock);

        if(num_thrd == 0)
            printf("not a valid number of threads");
        
        for(int i=1; i <num_thrd; ++i){
            pthread_join(thread[i],NULL);
        }

        
        fclose(fr1);
        fclose(fr2);
        
    }
    else
    {
        printf("Program terminated. Both matrices need to be the same size");
        exit(1);
    }
    
    //Once your output matrix is made, check if it is a standard output
    //or output to file
    
    if (toStdOut==1)
    {
        //Create the output file
        fw=fopen(argv[4],"w");
        //Check if the file has been created successfully or not
        if (fw==NULL) {
            printf("File not created. Program terminated...");
            exit(1);
        }
        int i,k;
        for( i = 0; i < rc1;++i)
        {
            for(k = 0; k<rc1; ++k)
            {
                fprintf(fw,"%d ", solution[k][i]);
            }
            fprintf(fw,"\n");
        }
        fclose(fw);
    }
    else
    {
        int i,k;
        for(i = 0; i < rc1;++i)
        {
            for(k = 0; k<rc1; ++k)
            {
                printf("%d ", solution[k][i]);
            }
            printf("\n");
        }
        
    }
    //end timer
    end = clock();
    time_spent = (double)(end - begin) / 1000;
    
    printf("\n%.4f seconds elapsed\n", time_spent);
    
    return 0;
}

//The following function will count the number of elements per line
//and return the number of rows and columns for the matrix
//int matSize(char *line)
//{
//    int count=0;
//    int i;
//    int length = strlen(line);
//    for( i=0; i < length;++i)
//    {
//        if(line[i] != ' ' && line[i] != '\n')
//            ++count;
//    }
//    return count;
//}

int matSize(char *line)
{
   int count=0;
   char delimeters[] = " ";
   char *token = strtok(line, delimeters);

   while((strcmp(token, "\n") != 0) && token != NULL)
   {
	   count++;
	   token =strtok(NULL, delimeters);
   }

   return count;
}

//The following function will take two matrices as input and return the
//product matrix
void matrixMultiply(void* params)
{
    struct matrixMultParam matrices = *(struct matrixMultParam*)params;
    int s = (int)matrices.slice;

    pthread_mutex_unlock(&lock);

    int from = (s*rc)/num_thrd;
    int to = ((s+1)*rc)/num_thrd;
    
    for(int i =from; i < to;++i)
    {
        for(int j=0;j<rc; ++j){
            matrices.solution[i][j] = 0;
            for(int k=0; k<rc; ++k){
                matrices.solution[i][j] += matrices.matrix1[i][k]*matrices.matrix2[k][j];
            }
        }
    }
}
//funtion to load up the matrices
void matrixLoad(int rc, int **mat, FILE* fr1)
{
    int val,i,k;
    
    rewind(fr1);
    for( i=0; i< rc; ++i)
    {
        for( k=0; k<rc; ++k)
        {
            fscanf(fr1,"%i", &val);
            mat[k][i] = val;
        }
    }
    
}
