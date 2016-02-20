/* Project 5 - Fall 2015
 * Name - Luka Antolic-Soban
 */

#include "cachesim.h"

typedef struct {
	unsigned char valid;
	unsigned char dirty;
	int tag;
} cache_block;

typedef struct {
	cache_block** blocks;
} cache_line;

cache_line* CACHE;
struct cache_stats_t* myStats;



int offset_bits();
int index_bits();
int tag_bits();
int calc_offset(uint64_t address);
int calc_index(uint64_t address);
int calc_tag(uint64_t address);

cache_block* locate_block(uint64_t address);
cache_block* locate_invalid_block(uint64_t address);
cache_block* locate_LRU_block(uint64_t address);

/*Cache variables given to us*/
uint64_t cache_C;
uint64_t cache_S;
uint64_t cache_B;
uint64_t cache_size;
uint64_t set_assoc;
uint64_t block_size;
uint64_t L;
/*****************************/


/**
 * Sub-routine for initializing your cache with the parameters.
 * You may initialize any global variables here.
 *
 * @param C The total size of your cache is 2^C bytes
 * @param S The set associativity is 2^S
 * @param B The size of your block is 2^B bytes
 */
void cache_init(uint64_t C, uint64_t S, uint64_t B) {

	cache_C = C;
	cache_S = S;
	cache_B = B;

	cache_size = pow(2,C);
	set_assoc = pow(2,S);
	block_size = pow(2,B);

	L = pow(2,C) / (pow(2,S) * pow(2,B));

	CACHE = (cache_line*) malloc(sizeof(cache_line) * L);

	for (int i = 0; i < L; i++)
	{
		CACHE[i].blocks = malloc(sizeof(cache_block) * set_assoc);

		for (int j = 0; j < set_assoc; j++)
		{
			CACHE[i].blocks[j] = malloc(sizeof(cache_block));
			CACHE[i].blocks[j]->valid = 0;
			CACHE[i].blocks[j]->dirty = 0;
			CACHE[i].blocks[j]->tag = 0;
		}
	}

	myStats = malloc(sizeof(struct cache_stats_t));

}

/**
 * Subroutine that simulates one cache event at a time.
 * @param rw The type of access, READ or WRITE
 * @param address The address that is being accessed
 * @param stats The struct that you are supposed to store the stats in
 */
void cache_access (char rw, uint64_t address, struct cache_stats_t *stats) {

	/*the cache was accessed*/
	myStats->accesses++;

	/*IT is a READ*/
	if(rw == 'r') {

		cache_block* block;

		/*this current access was a READ*/		
		myStats->reads++;

		/*we check if the block is in the cache*/
		block = locate_block(address);
		if(block == NULL) {

			/*this is a cache miss so we have to look for an invalid block*/
			block = locate_invalid_block(address);
			if(block == NULL) {

				/*There are no invalid blocks so we must evict LRU*/
				block = locate_LRU_block(address);

				
			}

			/*We save it, if it is dirty*/
			if(block->dirty) { myStats->write_backs++; }

			/* Initialize the block with new tag and metadata*/
			block->dirty = 0;
			block->valid = 1;
			block->tag = calc_tag(address);

			/* We count this as a cache miss*/
			myStats->read_misses++;
			return;
		}

		return;

	} /*end if READ*/








	if(rw == 'w') {

		cache_block* block;

		/*this current access was a WRITE*/
		myStats->writes++;

		/*we check if the block is in the cache*/
		block = locate_block(address);
		if(block == NULL) {
			/*we look for invalid blocks since we use write-back write-allocate*/
			block = locate_invalid_block(address);
			if(block == NULL) {

				/*Since there is no invalid block we evict the LRU*/
				block = locate_LRU_block(address);

				/*If block is dirty we count that as a write_back*/
				if(block->dirty) { myStats->write_backs++; }
			}

			/*initialize block with new tag and metadata*/
			block->dirty = 1;
			block->valid = 1;
			block->tag = calc_tag(address);

			/* We count this as a cache miss*/
			myStats->write_misses++;
			return;
		}

		/* This was a cache write hit so we set the dirty bit*/
		block->dirty = 1;
		return;

	} /* end if WRITE*/





}

/**
 * Subroutine for cleaning up memory operations and doing any calculations
 * Make sure to free malloced memory here.
 *
 */
void cache_cleanup (struct cache_stats_t *stats) {

	int i,j;

	for (i = 0; i < L; i++)
	{
		for (j = 0; j < set_assoc; j++)
		{
			free(CACHE[i].blocks[j]);
		}

		free(CACHE[i].blocks);
	}

	free(CACHE);

	stats->accesses = myStats->accesses;
	stats->reads = myStats->reads;
	stats->read_misses = myStats->read_misses;
	stats->writes = myStats->writes;
	stats->write_misses = myStats->write_misses;
	stats->misses = myStats->misses;
	stats->write_backs = myStats->write_backs;
	stats->misses = myStats->read_misses + myStats->write_misses;

	/* miss rate = 1 - hit rate --------- hit rate = */
	double read_hit = myStats->reads - myStats->read_misses;
	double write_hit = myStats->writes - myStats->write_misses;
	double hit_rate = (read_hit + write_hit) / myStats->accesses;
	stats->miss_rate = (1 - hit_rate);
	stats->avg_access_time = stats->access_time + (stats->miss_rate * stats->miss_penalty);


	free(myStats);
}




cache_block* locate_block(uint64_t address) {
	int i;
	int offset = calc_offset(address);
	offset = offset;
	int index = calc_index(address);
	int tag = calc_tag(address);

	cache_block* block;

	/*Loop through the cache line*/
	for(i = 0; i < set_assoc; i++) {
		/*Grab a block in the set for comparison*/
		block = CACHE[index].blocks[i];

		/* Check if valid and the tags match*/
		if((block->valid) && block->tag == tag) {

			/* Put the block in the front of the set and shift the others down*/
			for(int j = i; j > 0; j--) {
				CACHE[index].blocks[j] = CACHE[index].blocks[j-1];
			}

			CACHE[index].blocks[0] = block;

			return CACHE[index].blocks[0];
		}
	}
	/* Return NULL if there was no matching block found*/
	return NULL;

}

cache_block* locate_invalid_block(uint64_t address) {
	int i;
	int index = calc_index(address);
	cache_block* block;

	for(i=0; i < set_assoc; i++) {


		block = CACHE[index].blocks[i];

		if(!(block->valid)) {

			/* Put the block in the front of the set and shift the others down*/
			for(int j = i; j > 0; j--) {
				CACHE[index].blocks[j] = CACHE[index].blocks[j-1];
			}

			CACHE[index].blocks[0] = block;

			return CACHE[index].blocks[0];

		}

	}
	/*No invalid blocks were found*/
	return NULL;
}

cache_block* locate_LRU_block(uint64_t address) {
	int i;
	int index = calc_index(address);

	/*The LRU block is the one at the end of the list at the index*/
	cache_block* lru = CACHE[index].blocks[set_assoc-1];

	/*We now have to set it as used*/
	for(i=(set_assoc-1); i > 0; i--) {

		CACHE[index].blocks[i] = CACHE[index].blocks[i-1];
	}

	CACHE[index].blocks[0] = lru;

	return CACHE[index].blocks[0];
}














/*offset bits*/
int offset_bits() {
	return cache_B;
}

/*index bits*/
int index_bits() {
	return log2(L);
}

/*tag bits*/
int tag_bits() {
	return 64 - offset_bits() - index_bits();
}

/* Decode the offset from address*/
int calc_offset(uint64_t address) {
	int ob = offset_bits();
	return address & ((1 << ob) - 1);
}

/*Decode the index from the address*/
int calc_index(uint64_t address) {
	int ob = offset_bits();
	int ib = index_bits();
	return (address & (((1 << ib) - 1) << ob)) >> ob;
}

/*Decode the tag from address*/
int calc_tag(uint64_t address) {
	int ob = offset_bits();
	int ib = index_bits();
	int tb = tag_bits();

	if(tb == 64) {
		return address;
	}
	return (address & (((1 << tb) - 1) << ob << ib)) >> ob >> ib;
}