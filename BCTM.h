// Original Version was Created by Ole-Christoffer Granmo on 2019

#include "TM.h"

struct BinaryClassTsetlinMachine { 
	struct TsetlinMachine *tsetlin_machines[2];
};

struct BinaryClassTsetlinMachine *CreateBinaryClassTsetlinMachine();

void bc_tm_initialize(struct BinaryClassTsetlinMachine *bc_tm);

void bc_tm_initialize_random_streams(struct BinaryClassTsetlinMachine *bc_tm);

float bc_tm_evaluate(struct BinaryClassTsetlinMachine *bc_tm, unsigned int X[][LA_CHUNKS], int y[], int number_of_examples);

void bc_tm_fit(struct BinaryClassTsetlinMachine *bc_tm, unsigned int X[][LA_CHUNKS], int y[], int number_of_examples, int epochs);

void bc_tm_infer(struct BinaryClassTsetlinMachine *bc_tm);