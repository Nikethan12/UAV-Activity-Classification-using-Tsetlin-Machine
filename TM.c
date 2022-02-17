// Original Version was Created by Ole-Christoffer Granmo on 2019

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include "fast_rand.h"
#include "TM.h"

struct TsetlinMachine *CreateTsetlinMachine()
{
	// Create the Tsetlin Machine 

	struct TsetlinMachine *tm = (void *)malloc(sizeof(struct TsetlinMachine));

	tm_initialize(tm);

	return tm;
}

void tm_initialize(struct TsetlinMachine *tm)
{
	// initialize the Tsetlin Machine structure 

	for (int j = 0; j < CLAUSES; ++j) {
		for (int k = 0; k < LA_CHUNKS; ++k) {
			for (int b = 0; b < STATE_BITS-1; ++b) {											
				(*tm).ta_state[j][k][b] = ~0;
			}
			(*tm).ta_state[j][k][STATE_BITS-1] = 0;
		}
	}
}

void tm_initialize_random_streams(struct TsetlinMachine *tm)
{
	// Initialize all bits to zero	
	memset((*tm).feedback_to_la, 0, LA_CHUNKS*sizeof(unsigned int));

	// Generating Feedback to literals probabalistically
	int n = 2 * FEATURES;
	double p = 1.0 / S;
	int active = normal(n * p, n * p * (1 - p));
	active = active >= n ? n : active;
	active = active < 0 ? 0 : active;
	while (active--) { 
		int f = fast_rand() % (2 * FEATURES);
		while ((*tm).feedback_to_la[f / INT_SIZE] & (1 << (f % INT_SIZE))) {
			f = fast_rand() % (2 * FEATURES);
	    }
		(*tm).feedback_to_la[f / INT_SIZE] |= 1 << (f % INT_SIZE);
	}
}

// Increment the states of each of those 32 Tsetlin Automata flagged in the active bit vector.
void tm_inc(struct TsetlinMachine *tm, int clause, int chunk, unsigned int active)
{
	unsigned int carry, carry_next;

	carry = active;
	for (int b = 0; b < STATE_BITS; ++b) {
		if (carry == 0)
			break;

		carry_next = (*tm).ta_state[clause][chunk][b] & carry; // Sets carry bits (overflow) passing on to next bit
		(*tm).ta_state[clause][chunk][b] = (*tm).ta_state[clause][chunk][b] ^ carry; // Performs increments with XOR
		carry = carry_next;
	}

	if (carry > 0) {
		for (int b = 0; b < STATE_BITS; ++b) {
			(*tm).ta_state[clause][chunk][b] |= carry;
		}
	} 	
}

// Decrement the states of each of those 32 Tsetlin Automata flagged in the active bit vector.
void tm_dec(struct TsetlinMachine *tm, int clause, int chunk, unsigned int active)
{
	unsigned int carry, carry_next;

	carry = active;
	for (int b = 0; b < STATE_BITS; ++b) {
		if (carry == 0)
			break;

		carry_next = (~(*tm).ta_state[clause][chunk][b]) & carry; // Sets carry bits (overflow) passing on to next bit
		(*tm).ta_state[clause][chunk][b] = (*tm).ta_state[clause][chunk][b] ^ carry; // Performs increments with XOR
		carry = carry_next;
	}

	if (carry > 0) {
		for (int b = 0; b < STATE_BITS; ++b) {
			(*tm).ta_state[clause][chunk][b] &= ~carry;
		}
	} 
}

// Sum up the votes for each class 
int sum_up_class_votes(struct TsetlinMachine *tm)
{
	int class_sum = 0;

	for (int j = 0; j < CLAUSE_CHUNKS; j++) {
		class_sum += __builtin_popcount((*tm).clause_output[j] & 0x55555555); // 0101
		class_sum -= __builtin_popcount((*tm).clause_output[j] & 0xaaaaaaaa); // 1010
	}

	class_sum = (class_sum > THRESHOLD) ? THRESHOLD : class_sum;
	class_sum = (class_sum < -THRESHOLD) ? -THRESHOLD : class_sum;

	return class_sum;
}

/* Calculate the output of each clause using the actions of each Tsetline Automaton. */
void tm_calculate_clause_output(struct TsetlinMachine *tm, unsigned int Xi[], int predict)
{
	memset((*tm).clause_output, 0, CLAUSE_CHUNKS*sizeof(unsigned int));

	for (int j = 0; j < CLAUSES; j++) {
		unsigned int output = 1;
		unsigned int all_exclude = 1;
		for (int k = 0; k < LA_CHUNKS-1; k++) {
			output = output && ((*tm).ta_state[j][k][STATE_BITS-1] & Xi[k]) == (*tm).ta_state[j][k][STATE_BITS-1];

			if (!output) {
				break;
			}
			all_exclude = all_exclude && ((*tm).ta_state[j][k][STATE_BITS-1] == 0);
		}

		output = output &&
			((*tm).ta_state[j][LA_CHUNKS-1][STATE_BITS-1] & Xi[LA_CHUNKS-1] & FILTER) ==
			((*tm).ta_state[j][LA_CHUNKS-1][STATE_BITS-1] & FILTER);

		all_exclude = all_exclude && (((*tm).ta_state[j][LA_CHUNKS-1][STATE_BITS-1] & FILTER) == 0);

		output = output && !(predict == PREDICT && all_exclude == 1);
	
		if (output) {
			unsigned int clause_chunk = j / INT_SIZE;
			unsigned int clause_chunk_pos = j % INT_SIZE;

 			(*tm).clause_output[clause_chunk] |= (1 << clause_chunk_pos);
 		}
 	}
}



// Updating Binary Tsetlin Machine with one training example at a time.

void tm_update(struct TsetlinMachine *tm, unsigned int Xi[], int target)
{
	// Calculate Clause Output 
	tm_calculate_clause_output(tm, Xi, UPDATE);
	// Get Each class Sum
	int class_sum = sum_up_class_votes(tm);

	
	// Calculate feedback to clauses

	float p = (1.0/(THRESHOLD*2))*(THRESHOLD + (1 - 2*target)*class_sum);
	memset((*tm).feedback_to_clauses, 0, CLAUSE_CHUNKS*sizeof(int));
  	for (int j = 0; j < CLAUSES; j++) {
    	unsigned int clause_chunk = j / INT_SIZE;
        unsigned int clause_chunk_pos = j % INT_SIZE;

        (*tm).feedback_to_clauses[clause_chunk] |= (((float)fast_rand())/((float)FAST_RAND_MAX) <= p) << clause_chunk_pos;
    }

	for (int j = 0; j < CLAUSES; j++) {
		unsigned int clause_chunk = j / INT_SIZE;
		unsigned int clause_chunk_pos = j % INT_SIZE;

		if (!((*tm).feedback_to_clauses[clause_chunk] & (1 << clause_chunk_pos))) {
			continue;
		}
		
		if ((2*target-1) * (1 - 2 * (j & 1)) == -1) {
			if (((*tm).clause_output[clause_chunk] & (1 << clause_chunk_pos)) > 0) {
				// Type II Feedback ( only iff Clause output =1)

				for (int k = 0; k < LA_CHUNKS; ++k) {
					tm_inc(tm, j, k, (~Xi[k]) & (~(*tm).ta_state[j][k][STATE_BITS-1]));
				}
			}
		} else if ((2*target-1) * (1 - 2 * (j & 1)) == 1) {
			// Type I Feedback (Independent of Clause Output)

			tm_initialize_random_streams(tm);

			if (((*tm).clause_output[clause_chunk] & (1 << clause_chunk_pos)) > 0) {
				for (int k = 0; k < LA_CHUNKS; ++k) {
					#ifdef BOOST_TRUE_POSITIVE_FEEDBACK
		 				tm_inc(tm, j, k, Xi[k]);
					#else
						tm_inc(tm, j, k, Xi[k] & (~tm->feedback_to_la[k]));
					#endif
		 			
		 			tm_dec(tm, j, k, (~Xi[k]) & tm->feedback_to_la[k]);
				}
			} else {
				for (int k = 0; k < LA_CHUNKS; ++k) {
					tm_dec(tm, j, k, tm->feedback_to_la[k]);
				}
			}
		}
	}
}

int tm_score(struct TsetlinMachine *tm, unsigned int Xi[]) {
	// Calculate Clause Output 
	tm_calculate_clause_output(tm, Xi, UPDATE);
	// Get Each class Sum
	int class_sum = sum_up_class_votes(tm);
}


bool tm_action(struct TsetlinMachine *tm, int clause, int la)
{
	int la_chunk = la / INT_SIZE;
	int chunk_pos = la % INT_SIZE;

	return (((*tm).ta_state[clause][la_chunk][STATE_BITS-1] & (1 << chunk_pos)) > 0) ;
}

// Writing final clause states into a text file
void tm_infer(struct TsetlinMachine *tm)
{

	FILE * fp;

	fp = fopen("UAV_cluasestate.txt", "a");
	if (fp == NULL) {
		printf("Error opening\n");
		exit(EXIT_FAILURE);
	}
	int sum=0;

	for(int i=0;i<2000;i++) {
		for(int j=0;j<FEATURES*2;j++) {
			// printf("%d ",tm_action(tm,i,j));
			char str[20];    //create an empty string to store number
			int number = tm_action(tm,i,j);
			sum=sum + number;
			sprintf(str, "%d", number);
			fputs(str, fp) ;
			fputs(" ", fp) ;
		}
		// printf("\n");
		fputs("\n", fp) ;
	}
	printf("%d\n",sum);
	fclose(fp);

}


