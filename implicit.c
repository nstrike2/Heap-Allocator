/* Neetish Sharma, CS 107, Assignment 7 — Heap Allocator
 * This file contains an implicit allocator, which allocates memory for a client and hosts three client-facing features: mallocing, freeing, and reallocing.
 */
#include "allocator.h"
#include "debug_break.h"
#include <stdio.h>
#include <string.h>

// header struct
typedef struct header {
    size_t sizenstatus;
} header;

// variables
static void *segment_begin;
static size_t segment_size;
static void *segment_end;
static size_t free_blocks;

// helper functions
size_t roundup(size_t sz, size_t mult);
size_t extract_size(header *hdr);
void split_block_if_poss(header *hdr, size_t needed);
bool is_free (header *hdr);


/* Function: mynit
 * -----------------
 * This function initializes a heap given a starting pointer and heap size,
 * which is guaranteed to be a multiple of ALIGNMENT. The intialized heap is
 * one free block with a header of length heap_size - sizeof(header). Returns
 * true if heap is able to be initialized.
 */
bool myinit(void *heap_start, size_t heap_size) {

    // check if heap_size is larger than twice the alignment, as we need space for a header and a free space properly aligned
    if (heap_size < (ALIGNMENT * 2)) {
        return false;
    }
    
    header *init_header = heap_start;
    segment_begin = heap_start;

    // size of initial heap space (excluding header)
    segment_size = heap_size - sizeof(header);

    // counter of free blocks that is updated during freeing and specific cases of mallocing
    free_blocks = 1;

    // stores heap size in header, with last 3 bits designating free or alloc
    init_header->sizenstatus = segment_size;
    
    segment_end = (char *)heap_start + heap_size;
    
    return true;
}

/* Function: mymalloc
 * -----------------
 * Allocates new memory space with size of requested_size by iterating through every possible
 * block to see if a free block exists that is large enough to host the request.
 */
void *mymalloc(size_t requested_size) {

    // requested amount of memory to malloc has to be less than the max and greater than 0
    if (requested_size > MAX_REQUEST_SIZE || requested_size == 0) {
        return NULL;
    }

    // round up requested size to a properly aligned multiple
    size_t needed = roundup(requested_size, ALIGNMENT);
    
    header *header_iterator = segment_begin;
    
    while ((char *)header_iterator < (char *)segment_end) {
        if (is_free(header_iterator)) {
            // if enough space exists for an allocation
            if (extract_size(header_iterator) >= needed) {
                // if enough space exists for another allocation after allocating current block
                split_block_if_poss(header_iterator, needed);
                
                // allocate block
                header_iterator->sizenstatus += 1;
                free_blocks--;

                // pointer to payload
                char *return_ptr = (char *)header_iterator + sizeof(header);
                return return_ptr;
            }
        }

        // iterate
        header_iterator = (header *)((char *)header_iterator +  sizeof(header) + extract_size(header_iterator));     
    }
                
    
    return NULL;
}
/* Function: myfree
 * -----------------
 * When passed in a pointer to a specific spot in memory, frees that block.
 */
void myfree(void *ptr) {

    // no freeing if the pointer is NULL
    if (ptr == NULL) {
        return;
    }
    
    // add to the number of free blocks that exist
    free_blocks++;

    // change status of block to free
    header *newptr = (header *)((char *)ptr - sizeof(header));
    newptr->sizenstatus -= 1;
}

/* Function: myrealloc
 * -----------------
 * Reallocates existing memory to new memory of a new size by calling mymalloc.
 */
void *myrealloc(void *old_ptr, size_t new_size) {

    void *reallocated = NULL;
    
    // if pointer to block passed in is NULL, malloc a new_size
    if (old_ptr == NULL) {
        return mymalloc(new_size);
    // if new_size is 0, free the block being passed in
    } else if (new_size == 0) {
        myfree(old_ptr);
    // otherwise reallocate as normal
    } else {
        reallocated = mymalloc(new_size);
        memcpy(reallocated, old_ptr, new_size);
        myfree(old_ptr);
    }
    return reallocated;
}

/* Function: validate_heap
 * -----------------
 * Validate heap implmenets multiple checks to see if the heap is valid. The first check
 * is whether, after iterating sequentially across the heap to count the number of free blocks,
 * that number of free blocks matches up with the free_blocks counter that updated as we called
 * mymalloc, myrealloc, and myfree. The second check is to see whether the header from each
 * iteration is a valid one (meaning its last bit is either 0 or 1). The third checkis to see
 * if the total memory counted up from iterating sequentially is properly aligned. The fourth
 * check is to see if this total memory matches up with the inital heap_size given to us.
 */
bool validate_heap() {

    // sequential iterator
    header *header_iterator = segment_begin;

    // total bytes of memory in heap, accumulated after iterating over each block sequentially
    size_t total_mem = 0;

    // free block counter for sequential iteration
    size_t free_list = 0;

    // SEQUENTIAL ITERATION
    while ((char *)header_iterator < (char *)segment_end) {

        // if block is free, add to the free_list
        if (is_free(header_iterator)) {
            free_list++;
        }
        
        // increment total_mem
        total_mem += sizeof(header) + extract_size(header_iterator);

        // check for properly aligned and valid header, meaning that block size is a multiple of alignment and that least three significant bits are either 0 or 1
        if ( (header_iterator->sizenstatus & 0x7) > 2) {
            printf("Error! Header is misaligned, or status bit (LSB) is invalid.\n");
            breakpoint();
        }

        // iterate
        header_iterator = (header *)((char *)segment_begin + total_mem);
    }

    size_t heap_size = segment_size + sizeof(header);

    // checks to see if free block counter from sequential  iteration matches total number of free blocks from commands
    if (free_list != free_blocks) {
        printf("Free blocks don't match up!\n");
        breakpoint();
        return false;
    }

    // checks to see if total size of memory in heap is valid (a multiple of ALIGNMENT)
    if ((total_mem % ALIGNMENT) != 0) {
        printf("Misaligned memory!\n");
        breakpoint();
        return false;
    }

    // checks to see if total size of memory in heap is equal to total heap size
    if (total_mem != heap_size) {
        printf("Memory allocation overflow!\n");
        breakpoint();
        return false;
    }
    
    return true;
}

/* Function: dump_heap
 * -----------------
 * Iterates over the heap to print out the contents of the heap, which I chose to be the status of
 * each block (free or allocated) and the size of the block. This is purely for debugging.
 */
void dump_heap() {

    header *header_iterator = segment_begin;
    printf("Heap segment starts at address %p, ends at %p.\n", segment_begin, (char *)segment_begin + segment_size + sizeof(header));
    while ((char *)header_iterator < (char *)segment_end) {
        printf("Status is %lu.\n", (header_iterator->sizenstatus & 0x1));
        printf("Size is %lu.\n", header_iterator->sizenstatus & 0xfffffffe);
        header_iterator = (header *)((char *)(header_iterator) + sizeof(header) + (header_iterator->sizenstatus & 0xfffffffe));
    }      
}

// HELPER FUNCTIONS

/* Function: roundup
 * -----------------
 * This function rounds up the given number to the given multiple, which
 * must be a power of 2, and returns the result.  (you saw this code in lab1!).
 */
size_t roundup(size_t sz, size_t mult) {
    return (sz + mult - 1) & ~(mult - 1);
}

// get size of the block
size_t extract_size(header *hdr) {
    return (hdr->sizenstatus) & 0xfffffffe;
}

//checking if a block is free
bool is_free (header *hdr) {
    return ((hdr->sizenstatus) & 0x1) == 0;
}

// if block is large enough to host an allocation and another free block, splits block into two, with rightmost block being free block
void split_block_if_poss(header *hdr, size_t needed) {
    if (hdr->sizenstatus - needed >= sizeof(header) + ALIGNMENT) {
        size_t remaining = hdr->sizenstatus;
        hdr->sizenstatus = needed + (hdr->sizenstatus & 0x1);
        header *chopped_block = (header *)((char *)hdr + sizeof(header) + needed);
        chopped_block->sizenstatus = remaining - needed - sizeof(header);
        free_blocks++;
    }
}
