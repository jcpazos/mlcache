#ifndef _ucb1.h
#define _ucb1.h

struct UCB_struct;

double reward(double choice, double t);

void ucb1(int numActions, int trials /*might want to pass function pointer for reward in future*/);

#endif

