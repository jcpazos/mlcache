#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include "ucb1.h"

#define SCALEUP 100

int integerSqrt(int n) {
	int smallCandidate;
	int largeCandidate;
    if (n < 2) {
        return n;
    } else {
        smallCandidate = integerSqrt(n >> 2) << 1;
        largeCandidate = smallCandidate + 1;
        if (largeCandidate*largeCandidate > n) {
            return smallCandidate;
        } else {
            return largeCandidate;
        }
    }

}

//TODO: finish this
// find the log base 2 of 32-bit v
//might need to scale v up
int integerLog(uint32_t v) {
	int r;      // result goes here

	static const int MultiplyDeBruijnBitPosition[32] = 
	{
	  0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
	  8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
	};

	v |= v >> 1; // first round down to one less than a power of 2 
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;

	return MultiplyDeBruijnBitPosition[(uint32_t)(v * 0x07C4ACDDU) >> 27];
}


int upperBound(int step, int numPlays) {
	//indexing from 0
	return integerSqrt(SCALEUP*SCALEUP*(2*integerLog(step+1) / numPlays));
}

//TODO: this should take into account some kind of "popularity"
//for the page throughout the learning stage
int reward(double choice, double t, int hit) {
	return SCALEUP*hit;
}

int pull(struct UCB_struct* ucb, struct Cache* cache) {
	int action = -1;
	//get action that maximizes gain.
	//if training, just use best action overall
	int i = 0;
	if (cache == NULL) {
		action = 0;
		for (i=0;i<ucb->numActions;i++) {
			if (ucb->ucbs[i] > ucb->ucbs[action]) {
			action = i;
			}
		}
	//else use best action in cache
	} else {
		action = cache->blocks_array[0];
		for (i=0;i<cache->cache_size;i++) {
			if (cache->theUCB->ucbs[cache->blocks_array[i]] > cache->theUCB->ucbs[action]) {
				action = cache->blocks_array[i];
			}
		}
	}
	int theReward = reward(action, ucb->t,1);
	ucb->numPlays[action]+=SCALEUP;
	ucb->payoffSums[action]+= theReward;
	ucb->t+=SCALEUP;
	return action;
}

void updateUCB(struct UCB_struct* ucb) {
	//TODO: do we update all actions regardless of not being in cache?
	int i = 0;
	for (i=0; i<ucb->numActions;i++) {
		ucb->ucbs[i] = -ucb->payoffSums[i] - upperBound(ucb->t, ucb->numPlays[i])*ucb->numPlays[i];
	}
}

//This function should be called after a cache hit, it does two things:
//decrease weight of blocks in cache that weren't referenced
//increase weight of block in cache that was referenced
void updateInCache(int actionToReward, struct Cache* cache) {
	int i=0;
	for (i=0;i<cache->cache_size;i++) {
			int cacheBlock = cache->blocks_array[i];
			if (actionToReward != cacheBlock) {
				cache->theUCB->payoffSums[cacheBlock] += reward(cacheBlock, 0, -1);
			} else {
				cache->theUCB->payoffSums[actionToReward]+= reward(actionToReward, 0, 1);
				//cache->theUCB->numPlays[actionToReward]+=SCALEUP;;
				
			}
			cache->theUCB->t+=SCALEUP;
	}
	
}

//initialize the UCB
struct UCB_struct* ucb1(int numActions, int trials, struct Cache* cache /*might want to pass function pointer for reward in the future*/) {
	trials+=numActions;
	struct UCB_struct* newUCB = (struct UCB_struct*) malloc(sizeof(struct UCB_struct));
	newUCB->numActions = numActions;
	newUCB->trials = trials;
	newUCB->payoffSums = (int*) malloc(numActions*sizeof(int));
	newUCB->numPlays = (int*) malloc(numActions*sizeof(int));
	newUCB->ucbs = (int*) malloc(numActions*sizeof(int));

	int i = 0;
	for (i=0;i<newUCB->numActions;i++) {
		newUCB->payoffSums[i] = 0;
		newUCB->numPlays[i] = 1;
		newUCB->ucbs[i] = 0;
	}

	newUCB->t = 0;
	//initialize empirical sums
	for (newUCB->t=0;i<numActions;newUCB->t++) {
		newUCB->payoffSums[newUCB->t] = 0;
	}

	newUCB->t = numActions;
	cache->theUCB = newUCB;

	while (newUCB->t<trials*SCALEUP) {
		updateInCache(0, cache);
		updateUCB(newUCB);
		//pull(newUCB, 0);
	}

	printf("Probabilities are: [");
	for (i=0; i<numActions;i++){
		printf("%d", newUCB->ucbs[i]);
		if (i != newUCB->numActions - 1) {
			printf(", ");
		}
	}
	printf("]\n");
}


int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr,"ucb1:  usage:  trials\n");
		exit(1);
	}
	int trials = atoi(argv[1]);
	srand(time(NULL)); 


	struct Cache* cache = (struct Cache*)malloc(sizeof(struct Cache));
	cache->hits = 0;
	cache->misses = 0;
	cache->reads = 0;
	cache->writes = 0;
	cache->cache_size = CACHE_SIZE;
	cache->curr_size = 0;
	//cache->blocks = head;

	int i=0;
	for (i=0; i < CACHE_SIZE; i++) {
		cache->blocks_array[i] = i;
	}
	/*int* sqrtarr = (int*) malloc(100*sizeof(int));
	int* logarr = (int*) malloc(100*sizeof(int));

	int i = 0;
	printf("logarr\n");
	printf("[");
	for (i=0; i<100; i++) {
		logarr[i] = integerLog(10000*i);
		printf("%d, ", logarr[i]);

	}
	printf("]\n");

	printf("sqrtarr\n");
	printf("[");
	for (i=0; i<100; i++) {
		sqrtarr[i] = integerSqrt(10000*i);
		printf("%d, ", sqrtarr[i]);

	}
	printf("]\n");

	int logcoll = 0;
	for (i=1; i<100; i++) {
		if (logarr[i-1] == logarr[i]) {
			logcoll++;
		}
	}

	int sqrtcoll = 0;
	for (i=1; i<100; i++) {
		if (logarr[i-1] == logarr[i]) {
			logarr++;
		}
	}

	printf("log collisions: %d, sqrt collisions: %d\n", logcoll, sqrtcoll);*/
	ucb1(1000, trials, cache);
	return 0;
}