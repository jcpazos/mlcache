#ifndef _simple_cache
#define _simple_cache
#include "ucb1.h"

#define CACHE_SIZE 100

//Node for doubly-linked list
struct Node  {
	int blockNo;
	struct Node* next;
	struct Node* prev;
};

//Struct for cache
struct Cache {
    int hits;
    int misses;
    int reads;
    int writes;
    int cache_size;
    int block_size;
    int numLines;
    int write_policy;
    int curr_size;
    struct Node* blocks;
    int blocks_array[CACHE_SIZE];
    struct UCB_struct* theUCB;
};

#endif