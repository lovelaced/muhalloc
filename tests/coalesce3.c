/* check for coalesce free space */
#include <assert.h>
#include <stdlib.h>
#include "mem.h"

int main() {
   assert(Mem_Init(4096) == 0);
   void * ptr[5];
Mem_Dump();
   ptr[0] = Mem_Alloc(600);
   assert(ptr[0] != NULL);
Mem_Dump();
   ptr[1] = Mem_Alloc(600);
   assert(ptr[1] != NULL);
Mem_Dump();
   ptr[2] = Mem_Alloc(600);
   assert(ptr[2] != NULL);
Mem_Dump();
   ptr[3] = Mem_Alloc(600);
   assert(ptr[3] != NULL);
Mem_Dump();
   ptr[4] = Mem_Alloc(600);
   assert(ptr[4] != NULL);
Mem_Dump();
   while (Mem_Alloc(600) != NULL)
Mem_Dump();
    ;

   assert(Mem_Free(ptr[1]) == 0);
Mem_Dump();
   assert(Mem_Free(ptr[3]) == 0);
Mem_Dump();
   assert(Mem_Free(ptr[2]) == 0);
Mem_Dump();
   ptr[2] = Mem_Alloc(1800);
Mem_Dump();
   assert(ptr[2] != NULL);
Mem_Dump();
   exit(0);
}
