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
 
/**************************************************************************
 * SCZ.h - Function protoypes for User-Callable SCZ compression routines. *
 *  This file is useful to #include within applications that link SCZ     *
 *  functions, when SCZ library routines are pre-compiled into .o files.  *
 * SCZ-version 1.8	11-25-08					  *
 **************************************************************************/

#ifndef Z_INCLUDE
#define Z_INCLUDE 

#include <sstream>
using namespace std;
#define SCZ_VERSION 1.8
float scz_version=SCZ_VERSION;

/************************************************************************/
/* Scz_Compress_File - Compresses input file to output file.		*/
/*  First argument is input file name.  Second argument is output file	*/
/*  name.  The two file names must be different.			*/
/************************************************************************/
int Scz_Compress_File( char *infilename, char *outfilename );


/************************************************************************/
/* Scz_Compress_Buffer2File - Compresses character array input buffer	*/
/*  to an output file.  This routine is handy for applications wishing	*/
/*  to compress their output directly while saving to disk.		*/
/*  First argument is input array, second is the array's length, and	*/
/*  third is the output file name to store to.				*/
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
/************************************************************************/
int Scz_Compress_Buffer2Buffer( char *inbuffer, int N, stringstream &outbuffer, int lastbuf_flag );


/************************************************************************/
/* Scz_Decompress_File - Decompresses input file to output file.        */
/*  First argument is input file name.  Second argument is output file  */
/*  name.  The two file names must be different.                        */
/************************************************************************/
int Scz_Decompress_File( char *infilename, char *outfilename );


/***************************************************************************/
/* Scz_Decompress_File2Buffer - Decompresses input file to output buffer.  */
/*  This routine is handy for applications wishing to read compressed data */
/*  directly from disk and to uncompress the input directly while reading  */
/*  from the disk.             						   */
/*  First argument is input file name.  Second argument is output buffer   */
/*  with return variable for array length. 				   */
/*  This routine allocates the output array and passes back the size.      */ 
/**************************************************************************/
int Scz_Decompress_File2Buffer( char *infilename, stringstream &__outbuff__);


/*******************************************************************************/
/* Scz_Decompress_Buffer2Buffer - Decompresses input buffer to output buffer.  */
/*  This routine is handy for decompressing data on the fly, to reduce dynamic */
/*  memory or network traffic between applications.    			       */
/*  First argument is input array, second is the array's length, the third is  */
/*  the output buffer array, with return variable for array length.  	       */
/*  This routine allocates the output array and passes back the size.          */ 
/*******************************************************************************/
int Scz_Decompress_Buffer2Buffer( char *inbuffer, int N, stringstream &__outbuff__ );


void scz_cleanup();	/* Call after last SCZ call to free temporarily allocated memory. */


#endif // Z_INCLUDE