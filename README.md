## Thread-Safe Memory Allocation
The program implements thread-safe memory management in C. The user program can opt for coarse-grain and and fin-grain versions.

## Requirements
C99

## Installation 
Include my-malloc.h in the user program.
```
include "myMalloc.h"
```
Compile and run the program. This is a sample Makefile:
```
main:	main.o myMalloc.o myMalloc-helper.o
	gcc -o main main.o myMalloc.o myMalloc-helper.o -lpthread

main.o:	driver.c myMalloc.h
	gcc -c driver.c

myMalloc.o:	myMalloc.c myMalloc.h myMalloc-helper.h
	gcc -c myMalloc.c

myMalloc-helper.o:	myMalloc-helper.c myMalloc-helper.h
	gcc -c myMalloc-helper.c

clean:
	rm -f *.o main

```
## API 
### int my_init(int num_threads, int flag)
Using my_init API, the user program specifies the number of threads that will use the memory pool and the version. There are two versions that the user can use coarse-grain and fine grain.
Any user program must call my_init before creating threads or invoking my_malloc or my_free. 

Input
num_threads: number of cores
flag: version to use

Output
0 if normal. 1 if fails.

## void *my_malloc(int size)
my_malloc allocates a chunk of memory.

Input
size: number of bits to be allocated from the shared pool of memory

Output
pointer to the allocated memory chunk

## void my_free(void *ptr)
my_free frees the allocated memory and returns it to the memory pool.

Input
ptr: pointer to the chunk that needs to be deallocated





