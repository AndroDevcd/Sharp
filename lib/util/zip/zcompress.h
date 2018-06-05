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
 #ifndef _ZLIB_COMPRESS
 #define _ZLIB_COMPRESS
 
 #include "zcore.h"
 
 extern int *scz_freq2; 

 #define DECOMPRESSION_OVERLOAD (64 * 1020 * 64) * 8

/*------------------------------------------------------------*/
/* Add an item to a value-sorted list of maximum-length N.    */
/* Sort from largest to smaller values.  (A descending list.) */
/*------------------------------------------------------------*/
 void scz_add_sorted_nmax( struct scz_amalgam *list, unsigned char *phrase, int lngth, int value, int N );


/*------------------------------------------------------------*/
/* Add an item to a value-sorted list of maximum-length N.    */
/* Sort from smaller to larger values.  (An ascending list.)  */
/*------------------------------------------------------------*/
 void scz_add_sorted_nmin( struct scz_amalgam *list, unsigned char *phrase, int lngth, int value, int N );


/*----------------------------------------------------------------------*/
/* Analyze a buffer to determine the frequency of characters and pairs. */
/*----------------------------------------------------------------------*/
 void scz_analyze( struct scz_item *buffer0_hd, int *freq1, int *freq2 );




/*------------------------------------------------------*/
/* Compress a buffer, step.  Called iteratively.	*/
/*------------------------------------------------------*/
int scz_compress_iter( struct scz_item **buffer0_hd );





/*******************************************************************/
/* Scz_Compress - Compress a buffer.  Entry-point to Scz_Compress. */
/*  Compresses the buffer passed in.				   */
/* Pass-in the buffer and its initial size, as sz1. 		   */
/* Returns 1 on success, 0 on failure.				   */
/*******************************************************************/
 int Scz_Compress_Seg( struct scz_item **buffer0_hd, int sz1 );



 long Scz_get_file_length( FILE *fp );




/************************************************************************/
/* Scz_Compress_File - Compresses input file to output file.		*/
/*  First argument is input file name.  Second argument is output file	*/
/*  name.  The two file names must be different.			*/
/*									*/
/************************************************************************/
 int Scz_Compress_File( char *infilename, char *outfilename );
 
/************************************************************************/
/* Scz_Compress_Buffer2File - Compresses character array input buffer	*/
/*  to an output file.  This routine is handy for applications wishing	*/
/*  to compress their output directly while saving to disk.		*/
/*  First argument is input array, second is the array's length, and	*/
/*  third is the output file name to store to.				*/
/*  (It also shows how to compress and write data out in segments for	*/
/*   very large data sets.)						*/
/*									*/
/************************************************************************/
 int Scz_Compress_Buffer2File( unsigned char *buffer, int N, char *outfilename );



/************************************************************************/
/* Scz_Compress_Buffer2Buffer - Compresses character array input buffer	*/
/*  to another buffer.  This routine is handy for applications wishing	*/
/*  to compress data on the fly, perhaps to reduce dynamic memory or	*/
/*  network traffic between other applications.				*/
/*  First argument is input array, second is the array's length, the	*/
/*  third is the output buffer array, with return variable for array	*/
/*  length, and the final parameter is a continuation flag.  Set true	*/
/*  if last segment, otherwise set false if more segments to come.	*/
/*  This routine allocates the output array and passes back the size.	*/ 
/*									*/
/************************************************************************/
 int Scz_Compress_Buffer2Buffer( char *inbuffer, int N, char **outbuffer, int *M, int lastbuf_flag );
 
 #endif // _ZLIB_COMPRESS
 
 