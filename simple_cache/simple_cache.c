#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ucb1.h"

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
    int* blocks;    
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

void removeFromCacheUCB() {
	int i = 0;
	for(i=0;i<cache->cache_size;i++){
		if cache->UCB.pull();
	}
}

void writeToCacheUCB(int blockNo) {
	//if cache is at limit size, evict block with lowest ucb
	if (cache->curr_size == cache->cache_size) {
		removeFromCacheUCB();
		cache->curr_size--;
	}
	insertUCB(blockNo);
	cache->curr_size++;
}

struct UCB_struct* readFromCacheUCB(int blockNo) {
	int toRead = findBlock(blockNo);
	if (toRead == NULL) {
		//block to read isn't in cache, get it from memory
		cache->misses++;
		writeToCacheUBC(blockNo);
		cache->writes++;
		cache->reads++;
	} else {
		//block to read is in cache
		cache->hits++;
		bringToHead(toRead);
		cache->reads++;
	}
	return toRead;
}

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
	cache->cache_size = 1000;
	cache->curr_size = 0;
	cache->blocks = head;

	int i=0;
	//sequential reads, all misses;
	for (i=0; i<10000; i++) {
		readFromCache(i);
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
