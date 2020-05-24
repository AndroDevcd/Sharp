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
 #ifndef _ZLIB_STREAM
 #define _ZLIB_STREAM

#include <cstdio>

 struct SCZ_File
 {
  FILE *fptr;

  struct scz_item *buffer_hd, *buffer_tl, *rdptr;
  int bufsz, totalsz, eof, feof;
  char mode;
  unsigned char chksum;
 };



/************************************************************************/
/* Scz_File_Open - Open a file for reading or writing compressed data.	*/
/*  First argument is file-name.  Second argument is 'r' or 'w'.	*/
/*  Returns 0 on failure to open file.					*/
/************************************************************************/
 extern struct SCZ_File *Scz_File_Open( char *filename, char *mode );


/************************************************************************/
/* Scz_WriteString - Incrementally buffers data and compresses it to	*/
/*  an output file when appropriate.  This routine is handy for 	*/
/*  applications wishing to compress output while writing to disk.	*/
/*  Especially those creating only small amounts of data at a time.	*/
/*  First argument is SCZ_File pointer, as returned by Scz_File_Open(). */
/*  Second argument is input array, third is the array's length.	*/
/*									*/
/* This routine just adds to an scz buffer, until enough has accmulated */
/* to compress a block's worth.   After all writing is done, file must 	*/
/* be closed with Scz_File_Close().					*/
/************************************************************************/
int Scz_WriteString( struct SCZ_File *sczfile, unsigned char *buffer, int N );


/************************************************************************/
/* Scz_File_Close - Close a file after reading or writing compressed 	*/
/*  data.  Argument is scz_file to be closed.				*/
/************************************************************************/
void Scz_File_Close( struct SCZ_File *sczfile );



/************************************************************************/
/* Scz_Feof - Test for end of a compressed input file.			*/
/************************************************************************/
int Scz_Feof( struct SCZ_File *sczfile );

/************************************************************************/
/* Scz_ReadString - Incrementally reads compressed file, decompresses  	*/
/*  and returns the next line, xml-tag or xml-contents.			*/
/*  This routine is handy for applications wishing to read compressed 	*/
/*  data directly from disk and to uncompress the input directly while	*/
/*  reading from the disk.						*/
/*  First argument is SCZ_File pointer, as returned by Scz_File_Open(). */
/*  Second argument is the delimiter list. To return input by carriage- */
/*  return delineated lines, use "\n", to parse by XML tags, use "<>".	*/
/*  (Delimiters "<>" will be left in the returned tags.)		*/
/*  Third argument is null-terminated output character string, which    */
/*  must be pre-allocated.  This routine does not allocate the output   */
/*  string!  Forth parameter is the maximum length of returned strings, */
/*  IE. the length of the supplied output array.			*/
/************************************************************************/
int Scz_ReadString( struct SCZ_File *sczfile, char *delimiter, unsigned char *outbuffer, int maxN );

 
 
 #endif // _ZLIB_STREAM
 
 