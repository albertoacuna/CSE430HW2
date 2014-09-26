#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<string.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>

#define N 2048

typedef struct SharedData {
	int matrix1[N][N];
	int matrix2[N][N];
	int solution[N][N];
	int slice;
	pthread_mutex_t lock;
} sharedStruct;

//prototypes
void matrixMultiply(sharedStruct *Cms);
void matrixLoad(FILE *file, int matrix[N][N]);
int matSize(char *line);
sharedStruct* sharedMemoryAttach();
void sharedMemoryDetach();
void destroySharedMemory();
void printMatrix(int matriz[N][N]);
void matrixToFile(FILE *fw);

int rc;
int num_thrd;
sharedStruct *Cms; /* pointer to shared struct */
int structId; /* segment id for output matrix C */
int base_pid;
int charCount = 10000;

int main(int argc, char *argv[]) {
	num_thrd = atoi(argv[4]);

	FILE *fr1, *fr2, *fw; //File pointers
	//int **mat1, **mat2, **solution; //To hold the matrix from the input file
	int rc1, rc2; //To hold the number of rows and columns
	int toStdOut = 1; //This is a flag variable. 0 for file output,1 for std out
	char line[charCount]; //for extract a char of lines

	//Check for the number of command line arguments
	if (argc != 5) {
		//Print output to standard output
		toStdOut = 0;
	}

	//To read a file use the fopen() function to open it
	fr1 = fopen(argv[1], "r");

	//If the file fails to open the fopen() returns a NULL
	if (fr1 == NULL) {
		printf("Cannot open %s. Program terminated...", argv[1]);
		exit(1);
	}

	// Similar to the above method read the second file
	fr2 = fopen(argv[2], "r");
	if (fr2 == NULL) {
		printf("Cannot open %s. Program terminated...", argv[2]);
		exit(1);
	}

	//get matrice sizes and check if they are equal so they can be multiplied
	fgets(line, 10000, fr1);
	rc1 = matSize(line);
	fgets(line, 10000, fr2);
	rc2 = matSize(line);
	fclose(fr1);
	fclose(fr2);

	fr1 = fopen(argv[1], "r");

	//If the file fails to open the fopen() returns a NULL
	if (fr1 == NULL) {
		printf("Cannot open %s. Program terminated...", argv[1]);
		exit(1);
	}

	// Similar to the above method read the second file
	fr2 = fopen(argv[2], "r");
	if (fr2 == NULL) {
		printf("Cannot open %s. Program terminated...", argv[2]);
		exit(1);
	}

	if (rc1 == rc2) {
		rc = rc1;
		//memory allocation for arrays

		//-----------------------------------------------------------
		//load up both matricies
		if ((structId = shmget(IPC_PRIVATE, sizeof(sharedStruct),
		IPC_CREAT | 0666)) < 0) {
			perror("smget returned -1\n");
			error(-1, errno, " ");
			exit(-1);
		}

		if ((Cms = sharedMemoryAttach()) == (sharedStruct *) -1) {
			perror("Process shmat returned NULL\n");
			error(-1, errno, " ");
		}

		matrixLoad(fr1, Cms->matrix1);
		matrixLoad(fr2, Cms->matrix2);

		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
		pthread_mutex_init(&Cms->lock, &attr);

		base_pid = getpid();
		for (int i = 1; i < num_thrd; i++) {
			if (getpid() == base_pid)
				fork();
			else {
				if ((Cms = sharedMemoryAttach()) == (sharedStruct *) -1) {
					perror("Process shmat returned NULL\n");
					error(-1, errno, " ");
				}
				break;
			}
		}

		matrixMultiply(Cms);

		if (getpid() == base_pid) {
			for (int i = 1; i < num_thrd; i++) {
				wait(NULL);
			}
		} else {
			sharedMemoryDetach();
		}

		fclose(fr1);
		fclose(fr2);
	} else {
		printf("Program terminated. Both matrices need to be the same size");
		exit(1);
	}

	//Once your output matrix is made, check if it is a standard output
	//or output to file
	if (getpid() == base_pid) {
		if (toStdOut == 1) {
			//Create the output file
			fw = fopen(argv[3], "w");
			if (fw == NULL) {
				printf("File not created. Program terminated...");
				exit(1);
			}

			matrixToFile(fw);
			fclose(fw);

		} else {
			printMatrix(Cms->solution);

		}
	}

	pthread_mutex_destroy(&Cms->lock);
	sharedMemoryDetach();
	destroySharedMemory();

	return 0;
}

void printMatrix(int matrix[N][N]) {
	int i, k;
	for (i = 0; i < rc; ++i) {
		for (k = 0; k < rc; ++k) {
			printf("%d ", matrix[k][i]);
		}
		printf("\n");
	}
}

void matrixToFile(FILE *fw) {
	//Check if the file has been created successfully or not
	int i, k;
	for (i = 0; i < rc; ++i) {
		for (k = 0; k < rc; ++k) {
			fprintf(fw, "%d ", Cms->solution[i][k]);
		}
		fprintf(fw, "\n");
	}
}

int matSize(char *line) {
	int count = 0;
	char delimeters[] = " ";
	char *token = strtok(line, delimeters);

	while ((strcmp(token, "\n") != 0) && token != NULL) {
		count++;
		token = strtok(NULL, delimeters);
	}

	return count;
}

//The following function will take two matrices as input and return the
//product matrix
void matrixMultiply(sharedStruct *Cms) {
	pthread_mutex_lock(&Cms->lock);
	int s = Cms->slice;
	Cms->slice++;
	pthread_mutex_unlock(&Cms->lock);

	int from = (s * rc) / num_thrd;
	int to = ((s + 1) * rc) / num_thrd;
	for (int i = from; i < to; ++i) {
		for (int j = 0; j < rc; ++j) {
			Cms->solution[i][j] = 0;
			for (int k = 0; k < rc; ++k) {
				Cms->solution[i][j] += Cms->matrix1[i][k] * Cms->matrix2[k][j];
			}
		}
	}
}
//funtion to load up the matrices
//void matrixLoad(int *mat, FILE* fr1) {
//	int val, i, k;
//
//	rewind(fr1);
//	for (i = 0; i < rc; ++i) {
//		for (k = 0; k < rc; ++k) {
//			fscanf(fr1, "%i", &val);
//			mat[rc*i+k] = val;
//		}
//	}
//
//}

void matrixLoad(FILE *file, int matrix[N][N]) {
	char line[charCount];
	char delimeters[] = " ";
	char *cp;
	char *token;

	fgets(line, charCount, file);
	int x = 0;
	while (line != NULL && x < rc) {
		cp = strdup(line);
		token = strtok(cp, delimeters);
		int y = 0;
		while (token != NULL && y < rc)
		{

				matrix[y][x] = atoi(token);

				y++;
				token = strtok(NULL, delimeters);
		}

			x++;

			if (fgets(line, charCount, file) == NULL)
				break;
	}
}

sharedStruct* sharedMemoryAttach() {
	Cms = (sharedStruct*) shmat(structId, NULL, 0);
	return Cms;
}

void sharedMemoryDetach() {
	if (shmdt((void*) Cms) == -1) {
		perror("shmdt returned -1\n");
		error(-1, errno, " ");
	} else {
		exit(0);
	}
}

void destroySharedMemory() {
	if (shmctl(structId, IPC_RMID, NULL) == -1) {
		perror("shmctl returned -1\n");
		error(-1, errno, " ");
	}
}

