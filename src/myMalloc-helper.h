// CSc 422, Spring 2024
// Program 2 header file for sequential my_malloc-helper

// a Chunk is a integer (allocSize) along with prev and next pointers
// implicitly, after each of these, there is data of allocSize
#include <pthread.h>

typedef struct chunk
{
    int allocSize;
    struct chunk *prev;
    struct chunk *next;
} Chunk;

typedef struct memory_manager
{
    Chunk *free_small;
    Chunk *alloc_small;
    Chunk *free_large;
    Chunk *alloc_large;
    int count_small_chunks;
    int count_large_chunks;
} MemManager;

typedef struct list_manager
{
    MemManager *lists[8];
    int num_cores;
    int small_max;
    int large_max;
    Chunk *first; // overflow beginning
    Chunk *last; // overflow end
} ListManager;

// total amount of memory to allocate, and size of each small Chunk
#define SIZE_TOTAL 276672
#define SIZE_SMALL 64
#define SIZE_LARGE 1024
#define COARSE_GRAINED 1
#define FINE_GRAINED 2

Chunk *create_list();

Chunk *list_front(Chunk *list);

void insert_after(Chunk *item, Chunk *cur_item);

void insert_before(Chunk *item, Chunk *cur_item);

void list_append(Chunk *list, Chunk *item);

void list_push(Chunk *list, Chunk *item);

void unlink_item(Chunk *item);

int memory_check(void *pointer);

int is_full(int size, int list_number, ListManager *list_man);

void set_up_chunks(Chunk *list, void *mem, int num, int block_size);

void move_between_lists(Chunk *to_move, Chunk *from_list, Chunk *to_list);

Chunk *get_chunk(Chunk *free_list, Chunk *alloc_list);

void return_chunk(Chunk *free_list, Chunk *alloc_list, Chunk *to_free);

