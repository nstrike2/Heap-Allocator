/* Neetish Sharma
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

// node struct that includes a header and two node pointers
typedef struct node {
    header hdr;
    struct node *prev;
    struct node *next;
} node;

// variables
static void *segment_begin;
static size_t segment_size; // size of initial payload
static void *segment_end;
static size_t free_blocks; // take out if necessary
static node *first_freenode;
static size_t free_blocks;

// helper functions
size_t extract_size(node *newnode);
void split_block_if_poss(node *currnode, size_t needed);
void add_freeblock(node *newnode);
void remove_freeblock(node *newnode);
bool is_free (node *newnode);
void coalesce_right (node *newnode);
size_t roundup(size_t sz, size_t mult);
node *get_hdrptr(void *ptr);

/* Function: mynit
 * -----------------
 * This function initializes a heap given a starting pointer and heap size,
 * which is guaranteed to be a multiple of ALIGNMENT. The intialized heap is
 * one free block with a header of length heap_size - sizeof(header). Returns
 * true if heap is able to be initialized.
 */
bool myinit(void *heap_start, size_t heap_size) {

    if (heap_size < (ALIGNMENT * 3)) {
        return false;
    }
    
    first_freenode = heap_start;
    segment_begin = heap_start;
    segment_size = heap_size - sizeof(header);
    free_blocks = 1;

    // stores heap size in header, with last 3 bits designating free or alloc
    (first_freenode->hdr).sizenstatus = segment_size;
    first_freenode->prev = NULL;
    first_freenode->next = NULL;

    segment_end = (char *)heap_start + heap_size;
    
    return true;
    
}

/* Function: mymalloc
 * -----------------
 * Allocates new memory space with size of requested_size by iterating through a linked list of
 * free blocks to see if a free block exists that is large enough to host the request.
 */
void *mymalloc(size_t requested_size) {

    // requested amount of memory to malloc has to be less than the max and greater than 0
    if (requested_size > MAX_REQUEST_SIZE || requested_size == 0) {
        return NULL;
    }

    // round up requested size to a properly aligned multiple
    size_t needed = requested_size <= ALIGNMENT * 2 ? ALIGNMENT * 2 : roundup(requested_size, ALIGNMENT);

    node *currnode = first_freenode;
    
    while (currnode != NULL) {

        // if enough space exists for an allocation
        if (extract_size(currnode) >= needed) {
            // if enough space exists for another allocation after allocating current block
            split_block_if_poss(currnode, needed);

            // remove newly allocated block from freelist, decrementing number of free blocks
            remove_freeblock(currnode); 

            // allocate block by changing header
            (currnode->hdr).sizenstatus += 1;

            // return
            char *return_ptr = (char *)(currnode) + sizeof(header);
            return return_ptr;
        }

        // iterate
        currnode = currnode->next;
    }

    return NULL;
}

/* Function: myfree
 * -----------------
 * When passed in a pointer to a specific spot in memory, frees that block. Adds this free block
 * to the free list.
 */
void myfree(void *ptr) {

    // no freeing if the pointer is NULL
    if (ptr == NULL) {
        return;
    }

    // add new free block to top of the freelist
    void *head = (char *)(ptr) - sizeof(header);
    node *newnode = (node *)head;

    // free the block
    (newnode->hdr).sizenstatus -= 1;

    // add newfreeblock to the linked list, incrementing number of free blocks
    add_freeblock(newnode);

    // check if right neighbor is free and coalesce if necessary
    node *right_neighbor = (node *)((char *)(newnode) + sizeof(header) + extract_size(newnode));
    while ( (void *)right_neighbor != segment_end && is_free(right_neighbor)) {
        coalesce_right(newnode);
        //iterate
        right_neighbor = (node *)((char *)(newnode) + sizeof(header) + extract_size(newnode));
    }
}

/* Function: myrealloc
 * -----------------
 * Reallocates existing memory to new memory of a new size by calling mymalloc. Also hosts an
 * in-place realloc by coalescing right blocks until there is enough space to host the request.
 */
void *myrealloc(void *old_ptr, size_t new_size) {
    // if pointer to block passed in is NULL, malloc a new_size
    if (old_ptr == NULL) {
        return mymalloc(new_size);
    // if new_size is 0, free the block being passed in
    } else if (new_size == 0) {
        myfree(old_ptr);
        return NULL;
    } else {

    // otherwise reallocate as normal
    
    // roundup new_size requested as necessary
    size_t needed = new_size < ALIGNMENT * 2 ? ALIGNMENT * 2 : roundup(new_size, ALIGNMENT);

    node *currnode = get_hdrptr(old_ptr);

    // IN-PLACE REALLOC

    // new_size shrinks, stays equal, or enough padding exists to accomodate an expansion
    if (extract_size(currnode) >= needed) {
        // if enough space exists for another allocation after allocating current block
        split_block_if_poss(currnode, needed);
        return old_ptr;
    } else { // if client requests more space, check to see if you can coalesce (coalesces as many blocks as possible)
        node *right_neighbor = (node *)((char *)(currnode) + sizeof(header) + extract_size(currnode));
        while ( (void *)right_neighbor != segment_end && is_free(right_neighbor)) {
            coalesce_right(currnode);
            // check if coalescing provides enough space
            if (extract_size(currnode) >= needed) {
                // if coalesced more space than needed where after allocation, further space exists for another allocation
                split_block_if_poss(currnode, needed);
                
                return old_ptr;
            }

            // iterate
            right_neighbor = (node *)((char *)(currnode) + sizeof(header) + extract_size(currnode));
        }

        // at this point, not enough space to realloc in-place
    }

    // MOVE REALLOC
    
    void *reallocated = NULL;
    reallocated = mymalloc(new_size);
    memcpy(reallocated, old_ptr, new_size);
    myfree(old_ptr);
    
    return reallocated;
    
    return NULL;
    
    }
}

/* Function: validate_heap
 * -----------------
 * Validate heap implmenets multiple checks to see if the heap is valid. The first check
 * is whether, after iterating sequentially across the heap to count the number of free blocks,
 * that number of free blocks matches up with the free_blocks counter that updated as we called
 * mymalloc, myrealloc, and myfree. The second check is whether, after iterating across the linked
 * list of free blocks and counting the number of free blocks, that number of free blocks matches
 * with both the sequential free block counter and the free_blocks counter that updated as we
 * called mymalloc, myrealloc, and myfree. The third check is to see whether the header from each
 * iteration is a valid one (meaning its last bit is either 0 or 1). The fourth check is to see
 * if the total memory counted up from iterating sequentially is properly aligned. The fifth
 * check is to see if this total memory matches up with the inital heap_size given to us.
 */
bool validate_heap() {

    // total bytes of memory in heap, accumulated after iterating over each block sequentially
    size_t total_mem = 0;

    // sequential iterator
    node *seq_iterator = segment_begin;
    // free block counter for sequential iteration
    size_t free_seq_list = 0;

    // SEQUENTIAL ITERATION
    while ((void *)seq_iterator < segment_end) {

        // if block is free, add to the free_seq_list
        if (is_free(seq_iterator)) {
            free_seq_list++;
        }

        // check for properly aligned and valid header, meaning that block size is a multiple of alignment and that least three significant bits are either 0 or 1
        if ( ((seq_iterator->hdr).sizenstatus & 0x7) > 2) {
            printf("Error! Header is misaligned, or status bit (LSB) is invalid.\n");
        }

        // increment total_mem
        total_mem += sizeof(header) + extract_size(seq_iterator);

        // iterate
        seq_iterator = (node *)((char *)segment_begin + total_mem);
    }

    // linked list iterator
    node *currnode = first_freenode;
    // free block counter for linked list iteration
    size_t free_linked_list = 0;

    // LINKED LIST ITERATION
    while (currnode != NULL) {
        if (is_free(currnode)) {
            free_linked_list++;
        }
        
        // check if each free block is listed only once
        if (currnode == currnode->next) {
            printf("Free block counted twice!\n");
            breakpoint();
        }

        currnode = currnode->next;
    }
    size_t heap_size = segment_size + sizeof(header);

    // checks to see if free block counter from linked list iteration matches total number of free blocks from commands
    if (free_linked_list != free_blocks) {
        printf("Free blocks don't match up from linked list iteration!\n");
        breakpoint();
        return false;
    }

    // checks to see if free block counter from sequential  iteration matches total number of free blocks from commands
    if (free_seq_list != free_blocks) {
        printf("Free blocks don't match up from sequential iteration!\n");
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

    node *iterator = segment_begin;
    printf("Heap segment starts at address %p, ends at %p.\n", segment_begin, (char *)segment_begin + segment_size + sizeof(header));
    while ((void *)iterator < segment_end) {
        printf("Status is %s.\n", is_free(iterator) ? "free" : "allocated");
        printf("Size is %lu.\n", extract_size(iterator));
        iterator = (node *)((char *)(iterator) + sizeof(header) + extract_size(iterator));
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
size_t extract_size(node *newnode) {
    return ((newnode->hdr).sizenstatus) & 0xfffffffe;
}

// add a freeblock, incrementing number of free blocks
void add_freeblock(node *newnode) {

    // rewire pointers to add newnode to front of free list
    if (first_freenode) {
        first_freenode->prev = newnode;
        newnode->next = first_freenode;
        newnode->prev = NULL;
        first_freenode = newnode;
    } else { // if list is empty
        first_freenode = newnode;
    }

    free_blocks++;
}

// remove a freeblock, decrementing number of free blocks
void remove_freeblock(node *newnode) {
   if (newnode->prev) {
       (newnode->prev)->next = newnode->next;
   }

   if (newnode->next) {
       (newnode->next)->prev = newnode->prev;
   }

   // update head of freelist if necessary
   if (first_freenode == newnode) {
       first_freenode = newnode->next;
   }

   // decrement number of free blocks
   free_blocks--;
}

// if block is large enough to host an allocation and another free block, splits block into two, with rightmost block being free block
void split_block_if_poss(node *currnode, size_t needed) {
    if (extract_size(currnode) - needed >= sizeof(header) + ALIGNMENT * 2) {
        size_t remaining = extract_size(currnode);
                
        // update currnode_head while maintaining status in left block
        (currnode->hdr).sizenstatus = needed + ( ((currnode->hdr).sizenstatus) & 0x1);

        // chopped free block
        node *chopped_node = (node *)((char *)currnode + sizeof(header) + needed);

        // update chopped node size
        (chopped_node->hdr).sizenstatus = remaining - needed - sizeof(header);

        // add chopped node (freeblock) to freelist
        add_freeblock(chopped_node);
    }
}

//checking if a block is free
bool is_free (node *newnode) {
    return (((newnode->hdr).sizenstatus) & 0x1) == 0;
}

// if right neighbor of newnode is free, coalesces newnode and its neighbor into one freeblock
void coalesce_right (node *newnode) {
    node *right_neighbor = (node *)((char *)(newnode) + sizeof(header) + extract_size(newnode));
    
    // sever right neighbor from freelist
    remove_freeblock(right_neighbor);

    // update newnode's payload size
    size_t rightneighbor_size = extract_size(right_neighbor);
    (newnode->hdr).sizenstatus += sizeof(header) + rightneighbor_size;
}

// given a pointer to the payload, will get its header pointer and cast it to a node
node *get_hdrptr(void *ptr) {
    return (node *)((char *)(ptr) - sizeof(header));
}
