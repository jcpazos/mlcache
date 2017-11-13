#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ucb1.h"

#define CACHE_SIZE 1000

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

struct Cache* cache; //global cache
struct Node* head; //global head of list
struct Node* tail; //global tail of list

//Created a new node with data x
struct Node* newNode(int blockNo) {
	struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
	newNode->blockNo = blockNo;
	newNode->prev = NULL;
	newNode->next = NULL;
	return newNode;
}

//removes last node from tail
void removeFromTail() {
	struct Node* tmp = tail->prev;
	tmp->next = NULL;
	free(tail);
	tail = tmp;
}

//Inserts a Node with data x at the head of the list
void insertAtHead(struct Node* nodeToInsert) {
	if (head == NULL) {
		head = nodeToInsert;
		tail = nodeToInsert;
	} else {
		head->prev = nodeToInsert;
		nodeToInsert->next = head;
		head = nodeToInsert;
	}
}

//moves node blockNo to head
void bringToHead(struct Node* toHead) {
	toHead->prev->next = toHead->next;
	if (toHead->next) {
		toHead->next->prev = toHead->prev;
	}
	toHead->next = NULL;
	toHead->prev = NULL;
	insertAtHead(toHead);
}

struct Node* findNode(int blockNo) {
	struct Node* tmp = head;
	while (tmp != NULL) {
		if (tmp->blockNo == blockNo) {
			return tmp;
		}
		tmp = tmp->next;
	}
	return NULL;
}

/*writeToCache for LRU
void writeToCache(int blockNo) {
	struct Node* toInsert = newNode(blockNo);
	//if cache is at limit size, evict LRU block
	if (cache->curr_size == cache->cache_size) {
		removeFromTail();
		cache->curr_size--;
	}
	insertAtHead(toInsert);
	cache->curr_size++;
}*/

/*Functions for cache managing with UCB */
//-----------------------------------------------------------------------------
int findBlock(blockNo) {
	int i = 0;
	for (i=0; i < cache->curr_size; i++) {
		if (blockNo == cache->blocks_array[i]) {
			return i;
		}
	}
	return 0;
}

int findEmptyLine() {
	int i = 0;
	for (i=0; i <cache->curr_size; i++) {
		if (cache->bloc_array[i] == -1) {
			return i;
		}
	}
}

void insertUCB(int blockNo, int idx) {
	cache->blocks_array[idx] = blockNo;
}

int removeFromCacheUCB() {
	int idx = findBlock(pull(cache->theUCB, cache));
	cache->blocks_array[idx] = -1;
	return idx;
}

void writeToCacheUCB(int blockNo) {
	//if cache is at limit size, evict block with lowest ucb
	int toInsert = -1;
	if (cache->curr_size == cache->cache_size) {
		toInsert = removeFromCacheUCB();
		cache->curr_size--;	
	}
	if (toInsert == -1) {
		toInsert = findEmptyLine();
	}
	insertUCB(blockNo, toInsert);
	cache->curr_size++;
}

struct UCB_struct* readFromCacheUCB(int blockNo) {
	//TODO: should we also reward blocks that stay in the cache at each read?
	int toRead = findBlock(blockNo);
	if (toRead == NULL) {
		//block to read isn't in cache, get it from memory
		//TODO: reward last page removed
		cache->misses++;
		writeToCacheUCB(blockNo);
		cache->writes++;
	} else {
		//block to read is in cache
		//TODO: reward all pages in cache
		cache->hits++;
	}
	cache->reads++;
	return toRead;
}

// -----------------------------------------------------------------------------------

/*readFromCache for LRU
struct Node* readFromCache(int blockNo) {
	struct Node* toRead = findNode(blockNo);
	if (toRead == NULL) {
		//block to read isn't in cache, get it from memory
		cache->misses++;
		writeToCache(blockNo);
		cache->writes++;
		cache->reads++;
	} else {
		//block to read is in cache
		cache->hits++;
		bringToHead(toRead);
		cache->reads++;
	}
	return head;
}*/

void resetCache() {
	struct Node* tmp1 = head;
	struct Node* tmp2;
	while(tmp1 != NULL) {
		tmp2 = tmp1;
		tmp1 = tmp1->next;
		free (tmp2);
	}
	head = NULL;
	tail = NULL;
	cache->hits = 0;
	cache->misses = 0;
	cache->reads = 0;
	cache->writes = 0;
	cache->curr_size = 0;
	cache->blocks = head;
}

int main() {
	head = NULL;
	tail = NULL;
	cache = (struct Cache*)malloc(sizeof(struct Cache));
	cache->hits = 0;
	cache->misses = 0;
	cache->reads = 0;
	cache->writes = 0;
	cache->cache_size = CACHE_SIZE;
	cache->curr_size = 0;
	cache->blocks = head;

	int i=0;
	for (i=0; i < CACHE_SIZE; i++) {
		cache->blocks_array[i] = -1;
	}
	//sequential reads, all misses;
	for (i=0; i<10000; i++) {
		//readFromCache(i);
		readFromCacheUCB(i);
	}
	printf("All done, cache misses: %d, cache hits: %d, cache reads: %d, cache writes: %d \n", cache->misses, cache->hits, cache->reads, cache->writes);

	resetCache();
	//looping size of cache, cache_size misses, all rest hits
	for (i=0; i<10000; i++) {
		readFromCache(i%cache->cache_size);
	}

	printf("All done, cache misses: %d, cache hits: %d, cache reads: %d, cache writes: %d \n", cache->misses, cache->hits, cache->reads, cache->writes);
	return 0;

}
