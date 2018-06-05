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
 #ifndef _ZLIB_CORE
 #define _ZLIB_CORE


 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #define SCZ_MAX_BUF  16777215
 #define SCZFREELSTSZ 2048
 #define nil 0
 
 extern int sczbuflen;

 struct scz_item		/* Data structure for holding buffer items. */
 {
   unsigned char ch;
   struct scz_item *nxt;
 } ;

 extern struct scz_item *sczfreelist1;
 
 struct scz_amalgam	/* Data structure for holding markers and phrases. */
 {
   unsigned char phrase[2];
   int value;
 } ;

 struct scz_item2		/* Data structure for holding buffer items with index. */
 {
   unsigned char ch;
   int j;
   struct scz_item2 *nxt;
 } ;

 extern struct scz_item2 *sczphrasefreq[256], *scztmpphrasefreq, *sczfreelist2;

 struct scz_block_alloc		/* List for holding references to blocks of allocated memory. */
 {
  void *mem_block;
  struct scz_block_alloc *next_block;
 } ;

 extern scz_block_alloc *scz_allocated_blocks, *scz_tmpblockalloc;


 extern struct scz_item *new_scz_item();

 void sczfree( struct scz_item *tmppt );


 extern struct scz_item2 *new_scz_item2();

 void sczfree2( struct scz_item2 *tmppt );


 void scz_cleanup();


/*-----------------------*/
/* Add item to a buffer. */
/*-----------------------*/
 void scz_add_item( struct scz_item **hd, struct scz_item **tl, unsigned char ch );

#endif // _ZLIB_CORE
