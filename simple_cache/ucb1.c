#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include "simple_cache.c"

struct UCB_struct{
	int numActions;
	int trials;
	int t;
	double* payoffSums;
	int* numPlays;
	double* ucbs;
};


double upperBound(int step, int numPlays) {
	//indexing from 0
	return sqrt(2*log(step+1) / numPlays);
}


//TODO: this should take into account some kind of "popularity"
//for the page throughout the learning stage
double reward(double choice, double t) {
	//TODO: change this to be based on cache hit or miss.
	if (((double)rand() / (double)RAND_MAX) <= 0.5) {
		return -1;
	}
	else return 1;
}

double pull(struct UCB_struct* ucb) {
	//TODO: do we update all actions regardless of not being in cache?
	for (i=0; i<numActions;i++) {
		ucb->ucbs[i] = ucb->payoffSums[i]/ucb->numPlays[i] + upperBound(ucb->t, ucb->numPlays[i]);
	}
	int action = 0;
	//get action that maximizes gain. TODO: check if action is in cache currently before choosing it.
	for (i=0;i<numActions;i++) {
		if (ucb->ucbs[i] > ucb->ucbs[action]) {
			action = i;
		}
	}
	double theReward = reward(action, ucb->t);
	ucb->numPlays[action]++;
	ucb->payoffSums[action]+= theReward;
	ucb->t++;
}

struct UCB_struct* ucb1(int numActions, int trials /*might want to pass function pointer for reward in future*/) {
	struct UCB_struct* newUCB = (struct UCB_struct*) malloc(sizeof(struct UCB_struct));
	newUCB->numActions = numActions;
	newUCB->trials = trials;
	newUCB->payoffSums = (double*) malloc(numActions*sizeof(double));
	newUCB->numPlays = (int*) malloc(numActions*sizeof(int));
	newUCB->ucbs = (double*) malloc(numActions*sizeof(double));

	int i = 0;
	for (i=0;i<newUCB->numActions;i++) {
		newUCB->payoffSums[i] = 0;
		newUCB->numPlays[i] = 1;
		newUCB->ucbs[i] = 0;
	}

	newUCB->t = 0;
	//initialize empirical sums
	for (newUCB->t=0;i<numActions;newUCB->t++) {
		newUCB->payoffSums[newUCB->t] = reward(newUCB->t,newUCB->t);
	}

	newUCB->t = numActions;

	while (newUCB->t<trials) {
		pull(newUCB);
	}

	printf("Probabilities are: [");
	for (i=0; i<numActions;i++){
		printf("%lf", newUCB->payoffSums[i]);
		if (i != newUCB->numActions - 1) {
			printf(", ");
		}
	}
	printf("]\n");
}

/*
int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr,"ucb1:  usage:  trials\n");
		exit(1);
	}
	int trials = atoi(argv[1]);
	srand(time(NULL)); 
	ucb1(10, trials);
	return 0;
}*/