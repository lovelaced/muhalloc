/******************************************************************************
 * FILENAME: freetests.c
 * AUTHOR:   v
 * DATE:     04 May 2015 
 * PROVIDES: A user test for Mem_Free() as defined in mem.c
 * *****************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "mem.h"


int memsize = 4096;
int allocation = 4000;
void * ptr = NULL;

int main() {
  /* Make sure the memory initializes correctly */
  assert(Mem_Init(memsize) == 0);

  printf("Allocating %d from initialized memory.\n\n", allocation);
  /* Make an allocation, get a pointer, and print memory usage */
  ptr = Mem_Alloc(allocation);
  Mem_Dump();

  printf("Freeing %d from initialized memory.\n", allocation);
  /* Free the memory and make sure it exits with status 0 */
  assert(Mem_Free(ptr) == 0);
  Mem_Dump();
  puts("Checking if Mem_Free() handles bad pointers...");
  /* Check exit status for NULL and bad pointers */
  ptr = NULL;
  assert(Mem_Free(ptr) == -1);
  ptr = (int *)5;
  assert(Mem_Free(ptr) == -1);
  puts("...done.");
}   
