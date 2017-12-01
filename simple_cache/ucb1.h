#ifndef _ucb1
#define _ucb1
#include "simple_cache.h"

struct UCB_struct{
	int numActions;
	int trials;
	int t;
	int* payoffSums;
	int* numPlays;
	int* ucbs;
};

int reward(double choice, double t, int hit);

struct UCB_struct* ucb1(int numActions, int trials, struct Cache* cache /*might want to pass function pointer for reward in future*/);

void updateInCache(int actionToReward, struct Cache* cache);

int pull(struct UCB_struct* ucb, struct Cache* cache);

void updateUCB(struct UCB_struct* ucb);

#endif

