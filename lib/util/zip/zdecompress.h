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
 #ifndef _ZLIB_DECOMPRESS
 #define _ZLIB_DECOMPRESS
 
 
 #define DECOMPRESSION_OVERLOAD (64 * 1020 * 64) * 8
 
/****************************************************************/
/* Decompress - Decompress a buffer.  Returns 1 on success, 0 	*/
/*  if failure.							*/
/****************************************************************/
 int Scz_Decompress_Seg( struct scz_item **buffer0_hd );



/************************************************************************/
/* Scz_Decompress_File - Decompresses input file to output file.        */
/*  First argument is input file name.  Second argument is output file  */
/*  name.  The two file names must be different.                        */
/*                                                                      */
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
/*                                                                         */
/**************************************************************************/
 int Scz_Decompress_File2Buffer( char *infilename, char **outbuffer, int *M );





/*******************************************************************************/
/* Scz_Decompress_Buffer2Buffer - Decompresses input buffer to output buffer.  */
/*  This routine is handy for applications wishing to decompress data on the   */
/*  fly, perhaps to reduce dynamic memory or network traffic between other     */
/*  applications.                         				       */
/*  First argument is input array, second is the array's length, the third is  */
/*  the output buffer array, with return variable for array length.  	       */
/*  from the disk.             						       */
/*  This routine allocates the output array and passes back the size.          */ 
/*                                                                             */
/*******************************************************************************/
 int Scz_Decompress_Buffer2Buffer( char *inbuffer, int N, char **outbuffer, int *M );
 
 #endif // _ZLIB_DECOMPRESS