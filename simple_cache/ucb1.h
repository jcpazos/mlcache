#ifndef _ucb1.h
#define _ucb1.h

#include "simple_cache.c"

struct UCB_struct;

double reward(double choice, double t);

void ucb1(int numActions, int trials /*might want to pass function pointer for reward in future*/);

void updateInCache(int blockToIgnore, struct Cache* cache);

void updateUCB(struct UCB_struct* ucb);

#endif

