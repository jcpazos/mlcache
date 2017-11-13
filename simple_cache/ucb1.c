#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

double upperBound(int step, int numPlays) {
	//indexing from 0
	return sqrt(2*log(step+1) / numPlays);
}

double reward(double choice, double t) {
	//if we have a cache miss, don't add a reward
	if (rand() <= 0.5) {
		return 0;
	}
	//else add a reward of 0
	else return 1;
}

void ucb1(int numActions, int trials /*might want to pass function pointer for reward in future*/) {
	double* payoffSums = (double*) malloc(numActions*sizeof(double));
	int* numPlays = (int*) malloc(numActions*sizeof(int));
	double* ucbs = (double*) malloc(numActions*sizeof(double));

	int i = 0;
	for (i=0;i<numActions;i++) {
		payoffSums[i] = 0;
		numPlays[i] = 1;
		ucbs[i] = 0;
	}

	int t = 0;
	//initialize empirical sums
	for (t=0;i<numActions;t++) {
		payoffSums[t] = reward(t,t);
	}

	t = numActions;

	while (t<trials) {
		for (i=0; i<numActions;i++) {
			ucbs[i] = payoffSums[i]/numPlays[i] + upperBound(t, numPlays[i]);
		}
		int action = 0;
		for (i=0;i<numActions;i++) {
			if (ucbs[i] > ucbs[action]) {
				action = i;
			}
		}
		double theReward = reward(action, t);
		numPlays[action]++;
		payoffSums[action]+= theReward;
		t++;
	}

	printf('Probabilities are: [')
	for (int i=0; i<numActions;i++){
		printf("%d");
		if (i != numActions - 1;) {
			printf(", ");
		}
	}
	printf(']\n');
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr,"ucb1:  usage:  trials\n");
		exit(1);
	}
	int trials = atoi(argv[1]);
	srand(time(NULL)); 
	ucb1(10, trials);
	return 0;
}