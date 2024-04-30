#include <stdlib.h>
#include "myMalloc-helper.h"
#include <stdio.h>

/**
 * @author Umid Muzrapov
 * Course: CS422
 * Programming Assignment 2
 * Instructor: David Lowenthal
 * TA: Makayla Bennett
 * Due Date: April 11th at 8 am.
 *
 * Description: The program implements a thread-safe malloc and free.
 * In addition my_malloc and my_free, it offers my_init(numThreads, flag).
 * Using my_init API, the user program specifies the number of threads that will use
 * the memory pool and the version. There are two versions that the user can use
 * -- coarse-grain and fine grain.
 *
 * Operational Requirements:
 *  C99
 *  stdio.h
 *  stdlib.h
 *  my_malloc-helper.h
 *
 */

// represents a collection of thread-specific memory pools.
ListManager *list_man;
// used to maintain the program version
int is_sequential, version;
// used to determine the id for a new thread
int next_id = 0, is_overflow_seen = 0;
pthread_mutex_t  *lock;
// a shared memory pool
MemManager *overflow;
int *seen_ids;
pthread_key_t key;

/*******************This is a block of helper functions for my_init ******************/

/**
 * This function determines the version of the program to use.
 * @param num_cores number of cores
 * @param flag version the user desires to use
 */
static void determine_version(int num_cores, int flag)
{
    // if num_cores is 1, the program is sequential, no matter what flag is passed.
    if (num_cores == 1) is_sequential = 1;
    version = flag;
}

/**
 * This function initialized the elements to use the shared memory pool.
 * @return  1 if fails. 0 if normal.
 */
static int initialize_overflow()
{
    // initialize lock
    lock = malloc(sizeof(pthread_mutex_t));
    memory_check(lock);
    pthread_mutex_init(lock, NULL);
    // initialize the key
    pthread_key_create(&key, NULL);

    void *mem = malloc(SIZE_TOTAL);
    if (memory_check(mem)) return 1;
    overflow = (MemManager *) malloc(sizeof(MemManager));
    overflow->free_small = create_list();
    overflow->alloc_small = create_list();
    overflow->free_large = create_list();
    overflow->alloc_large = create_list();
    set_up_chunks(overflow->free_small, mem, list_man->small_max, SIZE_SMALL);
    set_up_chunks(overflow->free_large, mem + (sizeof(Chunk) + SIZE_SMALL) * list_man->small_max,
                  list_man->large_max, SIZE_LARGE);

    // keep the beginning and end of the stack for overflow list.
    list_man->first = mem;
    list_man->last =
            mem + (sizeof(Chunk) + SIZE_SMALL) * list_man->small_max + (sizeof(Chunk) + SIZE_LARGE) * list_man->large_max;
    return 0;
}

/**
 * The function initializes a collection of thread-specific memory pools, setting up the
 * metadata of list manager.
 * @param num_cores number of cores
 */
static void initialize_list_manager(int num_cores)
{
    int num_small, num_large;
    num_small = (SIZE_TOTAL / 2) / (SIZE_SMALL + sizeof(Chunk));
    num_large = (SIZE_TOTAL / 2) / (SIZE_LARGE + sizeof(Chunk));
    list_man =  malloc(sizeof(ListManager));
    list_man->num_cores = num_cores;
    list_man->large_max = num_large;
    list_man->small_max = num_small;

    // for each core, just initiate dummy head for the time being.
    for (int i = 0; i < list_man->num_cores; i++)
    {
        list_man->lists[i] = malloc(sizeof(MemManager));
        list_man->lists[i]->free_small = create_list();
        list_man->lists[i]->alloc_small = create_list();
        list_man->lists[i]->free_large = create_list();
        list_man->lists[i]->alloc_large = create_list();
    }
}

/**
 * Thi function marks each thread-specific list to its full capacity.
 * Used for the coarse-grain version.
 */
static void mark_lists_full()
{
    for (int i = 0; i < list_man->num_cores; i++)
    {
        list_man->lists[i]->count_small_chunks = list_man->small_max;
        list_man->lists[i]->count_large_chunks = list_man->large_max;
    }
}

/**
 * This function initializes thread-specific lists.
 * @return 0 if normal. 1 if error occurs.
 */
int initialize_lists()
{
    for (int i = 0; i < list_man->num_cores; i++)
    {
        void *mem = malloc(SIZE_TOTAL);
        if (memory_check(mem)) return 1;
        set_up_chunks(list_man->lists[i]->free_small, mem, list_man->small_max, SIZE_SMALL);
        set_up_chunks(list_man->lists[i]->free_large, mem + (sizeof(Chunk) + SIZE_SMALL) * list_man->small_max,
                      list_man->large_max, SIZE_LARGE);
        list_man->lists[i]->count_large_chunks=0;
        list_man->lists[i]->count_small_chunks=0;
    }

    return 0;
}

/**
 * This function initializes a list that keeps seen threads ids.
 * @param num_cores
 * @return
 */
static int initialize_seen_ids(int num_cores)
{
    seen_ids = malloc(sizeof(int) * num_cores);
    if (memory_check(seen_ids)) return 1;

    for (int i = 0; i < num_cores; i++)
    {
        seen_ids[i] = 0;
    }

    return 0;
}


/**
 * Any user program must call my_init before creating threads or invoking my_malloc or my_free.
 * Assume num_threads <= num of processors. It sets memory pools and synchronization primitives
 * based on the version and num_threads.
 * @param numThreads number of cores
 * @param flag version the user want to use
 * @return 0 if normal. 1 if fails.
 */
int my_init(int num_threads, int flag)
{
    determine_version(num_threads, flag);
    initialize_list_manager(num_threads);
    initialize_seen_ids(num_threads);

    if (version == COARSE_GRAINED) // if coarse-grain, mark per-core lists full.
    {
        mark_lists_full();
    } else if (version == FINE_GRAINED)
    {
        if (initialize_lists()) return 1; // if fine-grain, initialize memory pool for cores.
    }

    if (initialize_overflow()) return 1;

    return 0;
}

/**
 * The function gets the id of the current thread.
 * @return
 */
int get_id()
{
    if (is_sequential || version == COARSE_GRAINED) return 0;

    int *id_p = ((int *) pthread_getspecific(key));
    int id;

    // If no thread-specific data value is associated with key
    if (id_p == NULL)
    {
        pthread_mutex_lock(lock);
        int* allocatedID = malloc(sizeof (int));
        *allocatedID = next_id;
        pthread_setspecific(key, (const void *) (allocatedID));
        next_id++;

        id = *((int *) pthread_getspecific(key));
        //if (id!=next_id) printf("Sanity check failed.\n");
        pthread_mutex_unlock(lock);

    } else id = *id_p;

    return id;
}

/**
 * The function allocated a memory Chunk to the user program.
 * It is thread-safe and used with the overflow list.
 * @param size number bits user needs
 * @return pointer to the allocated memory
 */
void *my_malloc_synchronized(int size)
{
    Chunk *to_alloc;

    // touch overflow if it is the first malloc for overflow
    if (!is_overflow_seen){
        system("touch Overflow");
        is_overflow_seen = 1;
    }

    if (is_sequential)
    {
        if (size < 65) to_alloc = get_chunk(overflow->free_small, overflow->alloc_small);
        else to_alloc = get_chunk(overflow->free_large, overflow->alloc_large);
    } else // it is either fine-grain or coarse-grain then which needs locking
    {
        pthread_mutex_lock(lock);
        if (size < 65) to_alloc = get_chunk(overflow->free_small, overflow->alloc_small);
        else to_alloc = get_chunk(overflow->free_large, overflow->alloc_large);
        pthread_mutex_unlock(lock);
    }

    return to_alloc;
}

// my_malloc just needs to get the next Chunk and return a pointer to its data
// note the pointer arithmetic that makes sure to skip over our metadata and
// return the user a pointer to the data
void *my_malloc(int size)
{
    int id = get_id();
    Chunk *to_alloc;

    // create Id files only for fine-grained version and once
    if (!seen_ids[id] && version == FINE_GRAINED){
        char str[12];
        sprintf(str, "touch Id-%d\n", id+1);
        system(str);
        seen_ids[id]=1;
    }

    // get a Chunk
    if (size < 65)
    {
        // if thread-specific list is full, use the overflow list
        if (is_full(SIZE_SMALL, id, list_man))
        {
            to_alloc = my_malloc_synchronized(size);
        }
        else
        {
            to_alloc = get_chunk(list_man->lists[id]->free_small, list_man->lists[id]->alloc_small);
            list_man->lists[id]->count_small_chunks++;
        }

    } else
    {
        if (is_full(SIZE_LARGE, id, list_man))
        {
            to_alloc = my_malloc_synchronized(size);
        } else
        {
            to_alloc = get_chunk(list_man->lists[id]->free_large, list_man->lists[id]->alloc_large);
            list_man->lists[id]->count_large_chunks++;
        }
    }

    return ((void *) ((char *) to_alloc) + sizeof(Chunk));
}

/**
 * The function puts back the block back on the free list.
 * It is thread-safe and used with the overflow list.
 * @param size number bits user needs
 * @return pointer to the allocated memory
 */
void my_free_synchronized(Chunk *to_free)
{
    if (is_sequential){
        if (to_free->allocSize == SIZE_SMALL)
        {
            return_chunk(overflow->free_small, overflow->alloc_small, to_free);
        }else return_chunk(overflow->free_large, overflow->alloc_large, to_free);
    } else { // it is either fine-grain or coarse-grain then which needs synchronization
        pthread_mutex_lock(lock);
        if (to_free->allocSize == SIZE_SMALL){
            return_chunk(overflow->free_small, overflow->alloc_small, to_free);
        } else return_chunk(overflow->free_large, overflow->alloc_large, to_free);
        pthread_mutex_unlock(lock);
    }
}

// my_free just needs to put the block back on the free list
// note that this involves taking the pointer that is passed in by the user and
// getting the pointer to the beginning of the Chunk (so moving backwards Chunk bytes)
void my_free(void *ptr)
{
    // find the front of the Chunk
    Chunk *toFree = (Chunk *) ((char *) ptr - sizeof(Chunk));

    // if returned memory belong to the shared pool, use synchronized my free
    if (toFree <= list_man->last && toFree >= list_man->first)
    {
        my_free_synchronized(toFree);
    } else
    {
        int id = get_id();

        if (toFree->allocSize == SIZE_SMALL){
            return_chunk(list_man->lists[id]->free_small, list_man->lists[id]->alloc_small, toFree);
            list_man->lists[id]->count_small_chunks--;
        } else {
            return_chunk(list_man->lists[id]->free_large, list_man->lists[id]->alloc_large, toFree);
            list_man->lists[id]->count_large_chunks--;
        }
    }
}
