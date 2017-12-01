#ifndef _ucb1
#define _ucb1

#include "simple_cache.c"

struct UCB_struct;

int reward(double choice, double t);

struct UCB_struct* ucb1(int numActions, int trials /*might want to pass function pointer for reward in future*/);

void updateInCache(int actionToReward, struct Cache* cache);

void updateUCB(struct UCB_struct* ucb);

#endif

