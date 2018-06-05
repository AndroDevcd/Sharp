 /*
 * Copyright (C) 2015 The Scorpion Programming Language
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Portions of the code surrounded by "// Begin Dalvik code" and
 * "// END Delvik code" are copyrighted and licensed separately, as
 * follows:
 *
 * The core software depicted in this library goes to 
 * the creator of SCZ(Simple Compression Utilities and Library)
 *
 * (http://scz-compress.sourceforge.net/) November 26, 2008
 *
 */
 #include "zcore.h"
 #include <stdio.h>
 #include <stdlib.h>

#include "../../../stdimports.h"
 
 using namespace std;
 
 int sczbuflen = 4 * 1048576;
 
 scz_item *sczfreelist1=0;

 scz_item2 *sczphrasefreq[256], *scztmpphrasefreq, *sczfreelist2=0;

 scz_block_alloc *scz_allocated_blocks = 0, *scz_tmpblockalloc;

 struct scz_item *new_scz_item()
 {
 int j;
 struct scz_item *tmppt;

 if (sczfreelist1==nil)
  {
   sczfreelist1 = (struct scz_item *)malloc(SCZFREELSTSZ * sizeof(struct scz_item));
   tmppt = sczfreelist1;
   for (j=SCZFREELSTSZ-1; j!=0; j--) 
    {
     tmppt->nxt = tmppt + 1;	/* Pointer arithmetic. */
     tmppt = tmppt->nxt;
    }
   tmppt->nxt = nil;
   /* Record the block allocation for eventual freeing. */
   scz_tmpblockalloc = (struct scz_block_alloc *)malloc(sizeof(struct scz_block_alloc));
   scz_tmpblockalloc->mem_block = sczfreelist1;
   scz_tmpblockalloc->next_block = scz_allocated_blocks;
   scz_allocated_blocks = scz_tmpblockalloc;
  }
 tmppt = sczfreelist1;
 sczfreelist1 = sczfreelist1->nxt;
 return tmppt;
}

 void sczfree( struct scz_item *tmppt )
 {
  tmppt->nxt = sczfreelist1;
  sczfreelist1 = tmppt;
 }


 struct scz_item2 *new_scz_item2()
 {
 int j;
 struct scz_item2 *tmppt;

 if (sczfreelist2==nil)
  {
   sczfreelist2 = (struct scz_item2 *)malloc(SCZFREELSTSZ * sizeof(struct scz_item2));
   tmppt = sczfreelist2;
   for (j=SCZFREELSTSZ-1; j!=0; j--) 
    {
     tmppt->nxt = tmppt + 1;	/* Pointer arithmetic. */
     tmppt = tmppt->nxt;
    }
   tmppt->nxt = nil;
   /* Record the block allocation for eventual freeing. */
   scz_tmpblockalloc = (struct scz_block_alloc *)malloc(sizeof(struct scz_block_alloc));
   scz_tmpblockalloc->mem_block = sczfreelist2;
   scz_tmpblockalloc->next_block = scz_allocated_blocks;
   scz_allocated_blocks = scz_tmpblockalloc;
  }
 tmppt = sczfreelist2;
 sczfreelist2 = sczfreelist2->nxt;
 return tmppt;
}

 void sczfree2( struct scz_item2 *tmppt )
 {
  tmppt->nxt = sczfreelist2;
  sczfreelist2 = tmppt;
 }


 void scz_cleanup()	/* Call after last SCZ call to free temporarily allocated */
 {			/*  memory which will all be on the free-lists now. */
 while (scz_allocated_blocks!=0)
  {
   scz_tmpblockalloc = scz_allocated_blocks;
   scz_allocated_blocks = scz_allocated_blocks->next_block;
   free(scz_tmpblockalloc->mem_block);
   free(scz_tmpblockalloc);
  }
 sczfreelist1 = nil;
 sczfreelist2 = nil;
}


/*-----------------------*/
/* Add item to a buffer. */
/*-----------------------*/
 void scz_add_item( struct scz_item **hd, struct scz_item **tl, unsigned char ch )
 {
 struct scz_item *tmppt;

 tmppt = new_scz_item();
 tmppt->ch = ch;
 tmppt->nxt = 0;
 if (*hd==0) *hd = tmppt; else (*tl)->nxt = tmppt;
 *tl = tmppt;
}

 
 
 
 