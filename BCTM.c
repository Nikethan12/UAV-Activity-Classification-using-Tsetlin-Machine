// Original Version was Created by Ole-Christoffer Granmo on 2019

#include <stdio.h>
#include <stdlib.h>

#include "BCTM.h"


// Creating Binary Class Tsetlin Machine 
struct BinaryClassTsetlinMachine *CreateBinaryClassTsetlinMachine()
{

	struct BinaryClassTsetlinMachine *bc_tm;

	bc_tm = (void *)malloc(sizeof(struct BinaryClassTsetlinMachine));

	for (int i = 0; i < 2; i++) {
		bc_tm->tsetlin_machines[i] = CreateTsetlinMachine();
	}
	return bc_tm;
}
// Initialising Binary Class Tsetlin Machine
void bc_tm_initialize(struct BinaryClassTsetlinMachine *bc_tm)
{
	for (int i = 0; i < 2; i++) {
		tm_initialize(bc_tm->tsetlin_machines[i]);
	}
}

// Evaluating Binary Class Tsetlin Machine

float bc_tm_evaluate(struct BinaryClassTsetlinMachine *bc_tm, unsigned int X[][LA_CHUNKS], int y[], int number_of_examples)
{
	int errors;
	int max_class;
	int max_class_sum;

	errors = 0;
	for (int l = 0; l < number_of_examples; l++) {
		// Finding Max Class sum 

		max_class_sum = tm_score(bc_tm->tsetlin_machines[0], X[l]);
		max_class = 0;
		for (int i = 1; i < 2; i++) {	
			int class_sum = tm_score(bc_tm->tsetlin_machines[i], X[l]);
			if (max_class_sum < class_sum) {
				max_class_sum = class_sum;
				max_class = i;
			}
		}

		if (max_class != y[l]) {
			errors += 1;
		}
	}
	
	return 1.0 - 1.0 * errors / number_of_examples;
}



// Updating Binary Tsetlin Machine with one training example at a time.

void bc_tm_update(struct BinaryClassTsetlinMachine *bc_tm, unsigned int Xi[], int target_class)
{
	tm_update(bc_tm->tsetlin_machines[target_class], Xi, 1);
	int negative_target_class = 0;
	if(target_class == 0) {
		negative_target_class = 1;
	}

	tm_update(bc_tm->tsetlin_machines[negative_target_class], Xi, 0);
}

// Batch Mode Training

void bc_tm_fit(struct BinaryClassTsetlinMachine *bc_tm, unsigned int X[][LA_CHUNKS], int y[], int number_of_examples, int epochs)
{
	for (int epoch = 0; epoch < epochs; epoch++) {
		// Add shuffling here...		
		for (int i = 0; i < number_of_examples; i++) {
			bc_tm_update(bc_tm, X[i], y[i]);
		}
	}
}

// To get Final Clause States
void bc_tm_infer(struct BinaryClassTsetlinMachine *bc_tm)
{
		for (int i = 0; i < 2; i++) {	
			printf("For Class %d",i);
			tm_infer(bc_tm->tsetlin_machines[i]);	
		}	
}
