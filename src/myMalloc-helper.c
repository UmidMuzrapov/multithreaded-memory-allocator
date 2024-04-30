/**
 * @author Umid Muzrapov
 * Course: CS422
 * Programming Assignment 2 Helper Source
 * Instructor: David Lowenthal
 * TA: Makayla Bennett
 * Due Date: April 11th at 8 am.
 *
 * Description: The source contains helper code for my_malloc.c
 *
 * Operational Requirements:
 *  C99
 *  stdlib.h
 *  my_malloc-helper.h
 *
 */

#include <stdlib.h>
#include "myMalloc-helper.h"

// list routines: implements a circular, doubly-linked list with a dummy header node

// create the list; just allocate a dummy header node, and both next and prev
// point to the header node
Chunk *create_list()
{
    Chunk *dummy = (Chunk *) malloc(sizeof(Chunk));
    dummy->allocSize = -1;  // this is the header node, not a real list node
    dummy->next = dummy;
    dummy->prev = dummy;
    return dummy;
}

// insert cur_item into the list after item
void insert_after(Chunk *item, Chunk *cur_item)
{
    cur_item->next = item->next;
    cur_item->prev = item;
    cur_item->prev->next = cur_item;
    cur_item->next->prev = cur_item;
}

// insert cur_item into the list before item
void insert_before(Chunk *item, Chunk *cur_item)
{
    insert_after(item->prev, cur_item);
}

// insert Chunk at end of list
void list_append(Chunk *list, Chunk *item)
{
    insert_after(list->prev, item);
}

// insert Chunk at beginning of list
void list_push(Chunk *list, Chunk *item)
{
    insert_before(list->next, item);
}

// remove item from the list
void unlink_item(Chunk *item)
{
    item->prev->next = item->next;
    item->next->prev = item->prev;
}

// return the first list element
Chunk *list_front(Chunk *list)
{
    return list->next;
}

// end of list routines

// take a block of memory and logically divide it into chunks
// note that blocksize is here because it's necessary when you add
// large blocks
void set_up_chunks(Chunk *list, void *mem, int num, int block_size)
{
    int i;
    int chunkSize = sizeof(Chunk) + block_size;

    for (i = 0; i < num; i++)
    {
        // note below mem is cast to a (char *), so we are adding i * chunksize bytes
        Chunk *currentChunk = (Chunk *) ((char *) mem + +i * chunkSize);  // start position of this Chunk
        currentChunk->allocSize = block_size;
        list_append(list, currentChunk);  // put this Chunk at the end of the list
    }
}

// used to move a block from free list to allocated list or vice versa
void move_between_lists(Chunk *to_move, Chunk *from_list, Chunk *to_list)
{
    // remove to_move from the source list
    unlink_item(to_move);
    // now push it on to the destination list
    list_push(to_list, to_move);
}

// get a Chunk---grab first available Chunk, put on allocated list, and return pointer to the Chunk
Chunk *get_chunk(Chunk *free_list, Chunk *alloc_list)
{
    Chunk *toAlloc = list_front(free_list);
    move_between_lists(toAlloc, free_list, alloc_list);
    return toAlloc;
}

// return a Chunk from the allocated list to the free list
void return_chunk(Chunk *free_list, Chunk *alloc_list, Chunk *to_free)
{
    move_between_lists(to_free, alloc_list, free_list);
}

/**
 * The function provides a utility if the malloc operation was successful.
 * @param pointer pointer to the allocated memory
 * @return 0 if normal. 1 if error.
 */
int memory_check(void *pointer)
{
    if (pointer == NULL) return 1;
    else return 0;
}

/**
 * The function checks if the list is full.
 * @param size small or large
 * @param list_number id of thread the list belongs to
 * @param list_man a collection of thread-specific memory-pool
 * @return 1 if full. 0 if not.
 */
int is_full(int size, int list_number, ListManager *list_man)
{
    if (size == SIZE_SMALL)
    {
        if (list_man->lists[list_number]->count_small_chunks == list_man->small_max) return 1;
    } else
    {
        if (list_man->lists[list_number]->count_large_chunks == list_man->large_max) return 1;
    }

    return 0;
}





