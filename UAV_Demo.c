// Original Version was Created by Ole-Christoffer Granmo on 2019


#include "BCTM.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define EPOCHS 15
#define NUMBER_OF_TRAINING_EXAMPLES 320
#define NUMBER_OF_TEST_EXAMPLES 80

unsigned int X_train[NUMBER_OF_TRAINING_EXAMPLES][LA_CHUNKS];
int y_train[NUMBER_OF_TRAINING_EXAMPLES];

unsigned int X_test[NUMBER_OF_TEST_EXAMPLES][LA_CHUNKS];
int y_test[NUMBER_OF_TEST_EXAMPLES];

void read_file(void)
{
	FILE * fp;
	size_t len = 0;
	char * line = NULL; 
	const char *space = " ";
	char *token = NULL;

// Training Dataset

	for (int i = 0; i < NUMBER_OF_TRAINING_EXAMPLES; i++) {
		for (int j = 0; j < LA_CHUNKS; j++) {
			X_train[i][j] = 0;
		}
	}

	fp = fopen("UAV_train.txt", "r");
	if (fp == NULL) {
		printf("Error opening\n");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < NUMBER_OF_TRAINING_EXAMPLES; i++) {
		getline(&line, &len, fp);

		token = strtok(line, space);
		for (int j = 0; j < FEATURES; j++) {
			if (atoi(token) == 1) {
				int chunk_nr = j / INT_SIZE;
				int chunk_pos = j % INT_SIZE;
				X_train[i][chunk_nr] |= (1 << chunk_pos);
			} else {
				int chunk_nr = (j + FEATURES) / INT_SIZE;
				int chunk_pos = (j + FEATURES) % INT_SIZE;
				X_train[i][chunk_nr] |= (1 << chunk_pos);
			}
			token=strtok(NULL,space);
		}
		y_train[i] = atoi(token);
	}
	fclose(fp);

	// Test Dataset I

	for (int i = 0; i < NUMBER_OF_TEST_EXAMPLES; i++) {
		for (int j = 0; j < LA_CHUNKS; j++) {
			X_test[i][j] = 0;
		}
	}

	fp = fopen("UAV_test.txt", "r");
	if (fp == NULL) {
		printf("Error opening\n");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < NUMBER_OF_TEST_EXAMPLES; i++) {
		getline(&line, &len, fp);

		token = strtok(line, space);
		for (int j = 0; j < FEATURES; j++) {
			if (atoi(token) == 1) {
				int chunk_nr = j / INT_SIZE;
				int chunk_pos = j % INT_SIZE;
				X_test[i][chunk_nr] |= (1 << chunk_pos);
			} else {
				int chunk_nr = (j + FEATURES) / INT_SIZE;
				int chunk_pos = (j + FEATURES) % INT_SIZE;
				X_test[i][chunk_nr] |= (1 << chunk_pos);
			}
			token=strtok(NULL,space);
		}
		y_test[i] = atoi(token);
	}
	fclose(fp);

}

int main(void)
{	
	srand(time(NULL));
	clock_t start_total = clock();

	read_file();
	clock_t end_total = clock();
	double time_used = ((double) (end_total - start_total)) / CLOCKS_PER_SEC;
	printf("Loading Time: %.1f s\n", time_used);
	
	struct BinaryClassTsetlinMachine *bc_tm = CreateBinaryClassTsetlinMachine();
	float avg_train_accuracy = 0 ;
	float avg_accuracy = 0 ;

	for (int i = 0; i < EPOCHS; i++) {
		printf("\nEPOCH %d\n", i+1);

		clock_t start_total = clock();
		bc_tm_fit(bc_tm, X_train, y_train, NUMBER_OF_TRAINING_EXAMPLES, 1);
		clock_t end_total = clock();
		double time_used = ((double) (end_total - start_total)) / CLOCKS_PER_SEC;

		printf("Training Time: %.1f s\n", time_used);
		float train_accuracy = bc_tm_evaluate(bc_tm, X_train, y_train, NUMBER_OF_TRAINING_EXAMPLES);
		printf("Test Accuracy: %.2f\n", 100*train_accuracy);
		avg_train_accuracy += train_accuracy;
		printf("Average accuracy: %.2f\n", (100*avg_train_accuracy)/(i+1));

		start_total = clock();
		float test_accuracy = bc_tm_evaluate(bc_tm, X_test, y_test, NUMBER_OF_TEST_EXAMPLES);
		end_total = clock();
		time_used = ((double) (end_total - start_total)) / CLOCKS_PER_SEC;
		printf("Evaluation Time: %.1f s\n", time_used);
		printf("Test Accuracy: %.2f\n", 100*test_accuracy);
		avg_accuracy += test_accuracy;
		printf("Average accuracy: %.2f\n", (100*avg_accuracy)/(i+1));
	
	}
		bc_tm_infer(bc_tm);
	return 0;
}
