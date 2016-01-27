/******************************************************************************
 * FILENAME: mem.c
 * AUTHOR:   cherin@cs.wisc.edu <Cherin Joseph>
 * DATE:     20 Nov 2013
 * PROVIDES: Contains a set of library functions for memory allocation
 * MODIFIED BY:  v
 * *****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "mem.h"

/* this structure serves as the header for each block */
typedef struct block_hd{
  /* The blocks are maintained as a linked list */
  /* The blocks are ordered in the increasing order of addresses */
  struct block_hd* next;

  /* Size of the block is always a multiple of 4 */
  /* Ie, last two bits are always zero - can be used to store other information*/
  /* LSB = 0 => free block */
  /* LSB = 1 => allocated/busy block */

  /* So for a free block, the value stored in size_status will be the same as the block size*/
  /* And for an allocated block, the value stored in size_status will be one more than the block size*/

  /* The value stored here does not include the space required to store the header */

  /* Example: */
  /* For a block with a payload of 24 bytes (ie, 24 bytes data + an additional 8 bytes for header) */
  /* If the block is allocated, size_status should be set to 25, not 24!, not 23! not 32! not 33!, not 31! */
  /* If the block is free, size_status should be set to 24, not 25!, not 23! not 32! not 33!, not 31! */
  int size_status;

}block_header;

/* Global variable - This will always point to the first block */
/* ie, the block with the lowest address */
block_header* list_head = NULL;


/* Function used to Initialize the memory allocator */
/* Not intended to be called more than once by a program */
/* Argument - sizeOfRegion: Specifies the size of the chunk which needs to be allocated */
/* Returns 0 on success and -1 on failure */
int Mem_Init(int sizeOfRegion)
{
  int pagesize;
  int padsize;
  int fd;
  int alloc_size;
  void* space_ptr;
  static int allocated_once = 0;
  
  if(0 != allocated_once)
  {
    fprintf(stderr,"Error:mem.c: Mem_Init has allocated space during a previous call\n");
    return -1;
  }
  if(sizeOfRegion <= 0)
  {
    fprintf(stderr,"Error:mem.c: Requested block size is not positive\n");
    return -1;
  }

  /* Get the pagesize */
  pagesize = getpagesize();

  /* Calculate padsize as the padding required to round up sizeOfRegio to a multiple of pagesize */
  padsize = sizeOfRegion % pagesize;
  padsize = (pagesize - padsize) % pagesize;

  alloc_size = sizeOfRegion + padsize;

  /* Using mmap to allocate memory */
  fd = open("/dev/zero", O_RDWR);
  if(-1 == fd)
  {
    fprintf(stderr,"Error:mem.c: Cannot open /dev/zero\n");
    return -1;
  }
  space_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (MAP_FAILED == space_ptr)
  {
    fprintf(stderr,"Error:mem.c: mmap cannot allocate space\n");
    allocated_once = 0;
    return -1;
  }
  
  allocated_once = 1;
  
  /* To begin with, there is only one big, free block */
  list_head = (block_header*)space_ptr;
  list_head->next = NULL;
  /* Remember that the 'size' stored in block size excludes the space for the header */
  list_head->size_status = alloc_size - (int)sizeof(block_header);
  
  return 0;
}


/* Function for allocating 'size' bytes. */
/* Returns address of allocated block on success */
/* Returns NULL on failure */
/* Here is what this function should accomplish */
/* - Check for sanity of size - Return NULL when appropriate */
/* - Round up size to a multiple of 4 */
/* - Traverse the list of blocks and allocate the best free block which can accommodate the requested size */
/* -- Also, when allocating a block - split it into two blocks when possible */
/* Tips: Be careful with pointer arithmetic */
void* Mem_Alloc(int size)
{
  /* Your code should go in here */
  /* Set the current block to the first block */
  block_header* curr_block = list_head;
  /* The size of the current block */
  unsigned int curr_size = curr_block->size_status;
  /* Set the current difference to effective infinity */
  unsigned int difference = 2147483647;
  /* Initialize the best block found as NULL */
  block_header* best_block = NULL;

  /* While the size isn't a multiple of 4, increment it */
  while (size % 4 != 0) {
    size++;
  }
  /* If the current block is large enough and is free */
  if (curr_block->size_status >= size && curr_size%2 == 0) {
  /* Set the best block as the current block */ 
    best_block = curr_block;
  /* And update the difference */
    difference = curr_size-size;
  }   
  /* Iterate through the list of blocks */
  while (curr_block->next != NULL) {
  /* Since we've already checked the first block, immediately move to the next */
    curr_block = curr_block->next;
    curr_size = curr_block->size_status;
  /* Check if the current block is a better fit than the best block */
    if (difference >= (curr_size - size) && curr_size >= size && curr_size%2 == 0) {
  /* If it is, update the best block */   
      difference = curr_size - size;
      best_block = curr_block;
    }
  }

  /* If we've gotten through the list without finding a block, return NULL */
  if (best_block == NULL || best_block->size_status < size) {
    return NULL;
  }
  /* If the best block is larger than the size requested, split it */
  if (best_block->size_status > size) {
  /* Make a new block */
    block_header* new_block =(block_header*)(best_block + (size+sizeof(block_header))/4);
  /* Update pointers and size */
    new_block->next = best_block->next;
    best_block->next = new_block;
    best_block->next->size_status = best_block->size_status - size - sizeof(block_header);
  }
  /* Mark the block as busy */
  best_block->size_status = size+1;
  return best_block;    
}

/* Function for freeing up a previously allocated block */
/* Argument - ptr: Address of the block to be freed up */
/* Returns 0 on success */
/* Returns -1 on failure */
/* Here is what this function should accomplish */
/* - Return -1 if ptr is NULL */
/* - Return -1 if ptr is not pointing to the first byte of a busy block */
/* - Mark the block as free */
/* - Coalesce if one or both of the immediate neighbours are free */
int Mem_Free(void *ptr)
{
  /* Your code should go in here */
  /* If the pointer is NULL, exit */
  if (ptr == NULL) {
    return -1;
  }
  /* Initialize blocks to store */
  block_header* curr_block = list_head;
  block_header* prev_block = NULL;
  block_header* found_block = NULL; 
  /* Iterate through the blocks until we find the right one */
  while (ptr != curr_block) {
  /* Save the current block */
    prev_block = curr_block;
  /*  if we've reached the end of the list without finding the block, return -1 */
    if (curr_block->next == NULL) {
      return -1;
    }
  /* Make the current block the next block */
    curr_block = curr_block->next;
  }
  /* Save the block we found and set its status to free */ 
  found_block = ptr;
  curr_block = found_block;
  found_block->size_status--;
  /* Coalesce the previous block if it's free */
  if (prev_block != NULL) {
    if (prev_block->size_status%2==0) { 
      prev_block->size_status = found_block->size_status + 
        prev_block->size_status + sizeof(block_header);
      prev_block->next = found_block->next;
      found_block = prev_block;
    }
  }
  /* Coalesce the next block if it's free */
  if (curr_block->next != NULL) {
    if (curr_block->next->size_status%2==0) { 
      found_block->size_status = found_block->size_status + 
        curr_block->next->size_status + sizeof(block_header);
      found_block->next = curr_block->next->next;
    }
  }
  
  return 0;   
}

/* Function to be used for debug */
/* Prints out a list of all the blocks along with the following information for each block */
/* No.      : Serial number of the block */
/* Status   : free/busy */
/* Begin    : Address of the first useful byte in the block */
/* End      : Address of the last byte in the block */
/* Size     : Size of the block (excluding the header) */
/* t_Size   : Size of the block (including the header) */
/* t_Begin  : Address of the first byte in the block (this is where the header starts) */
void Mem_Dump()
{
  int counter;
  block_header* current = NULL;
  char* t_Begin = NULL;
  char* Begin = NULL;
  int Size;
  int t_Size;
  char* End = NULL;
  int free_size;
  int busy_size;
  int total_size;
  char status[5];

  free_size = 0;
  busy_size = 0;
  total_size = 0;
  current = list_head;
  counter = 1;
  fprintf(stdout,"************************************Block list***********************************\n");
  fprintf(stdout,"---------------------------------------------------------------------------------\n");
  while(NULL != current)
  {
    t_Begin = (char*)current;
    Begin = t_Begin + (int)sizeof(block_header);
    Size = current->size_status;
    strcpy(status,"Free");
    if(Size & 1) /*LSB = 1 => busy block*/
    {
      strcpy(status,"Busy");
      Size = Size - 1; /*Minus one for ignoring status in busy block*/
      t_Size = Size + (int)sizeof(block_header);
      busy_size = busy_size + t_Size;
    }
    else
    {
      t_Size = Size + (int)sizeof(block_header);
      free_size = free_size + t_Size;
    }
    End = Begin + Size;
    fprintf(stdout,"%d\t%s\t0x%08lx\t0x%08lx\t%d\t%d\t0x%08lx\n",counter,status,(unsigned long int)Begin,(unsigned long int)End,Size,t_Size,(unsigned long int)t_Begin);
    total_size = total_size + t_Size;
    current = current->next;
    counter = counter + 1;
  }
  fprintf(stdout,"---------------------------------------------------------------------------------\n");
  fprintf(stdout,"*********************************************************************************\n");

  fprintf(stdout,"Total busy size = %d\n",busy_size);
  fprintf(stdout,"Total free size = %d\n",free_size);
  fprintf(stdout,"Total size = %d\n",busy_size+free_size);
  fprintf(stdout,"*********************************************************************************\n");
  fflush(stdout);
  return;
}
