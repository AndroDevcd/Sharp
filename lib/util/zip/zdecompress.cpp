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
 #include "zdecompress.h"
 #include "zlib.h"
 #include <sstream>
 #include <iostream>
 using namespace std;


unsigned long DECOMPRESS_WATCHDOG = 0;

/****************************************************************/
/* Decompress - Decompress a buffer.  Returns 1 on success, 0 	*/
/*  if failure.							*/
/****************************************************************/
 int Scz_Decompress_Seg( struct scz_item **buffer0_hd )
 {
 unsigned char forcingchar, marker[257], phrase[256][256];
 int j, nreplaced, iterations, iter, markerlist[256];
 struct scz_item *bufpt, *tmpbuf, *oldptr;
 unsigned char ch;

 /* Expect magic number(s). */
 bufpt = *buffer0_hd;
 if ((bufpt==0) || (bufpt->ch!=101))
  {
   zres.reason << "zlib:  error: This does not look like a compressed file.\n"; 
   return ZLIB_FAILURE;
  }
 oldptr = bufpt;   bufpt = bufpt->nxt;   sczfree(oldptr);
 if ((bufpt==0) || (bufpt->ch!=98))
  {
   zres.reason << "zlib:  error: This does not look like a compressed file.\n";  
   return ZLIB_FAILURE;
  }
 oldptr = bufpt;   bufpt = bufpt->nxt;    sczfree(oldptr);

 /* Get the compression iterations count. */
 iterations = bufpt->ch;
 oldptr = bufpt;   bufpt = bufpt->nxt;   sczfree(oldptr);
 *buffer0_hd = bufpt;

 for (iter=0; iter<iterations; iter++)
  { /*iter*/
    bufpt = *buffer0_hd;
    forcingchar = bufpt->ch;	oldptr = bufpt;  bufpt = bufpt->nxt;  sczfree(oldptr);
    nreplaced = bufpt->ch;	oldptr = bufpt;  bufpt = bufpt->nxt;  sczfree(oldptr);
    for (j=0; j<nreplaced; j++)   /* Accept the markers and phrases. */
     {
      marker[j] =    bufpt->ch;	  oldptr = bufpt;  bufpt = bufpt->nxt;  sczfree(oldptr);
      phrase[j][0] = bufpt->ch;	  oldptr = bufpt;  bufpt = bufpt->nxt;  sczfree(oldptr);
      phrase[j][1] = bufpt->ch;	  oldptr = bufpt;  bufpt = bufpt->nxt;  sczfree(oldptr);
     }
    ch = bufpt->ch; 		oldptr = bufpt;  bufpt = bufpt->nxt;  sczfree(oldptr);
    if (ch!=91) /* Boundary marker. */
     {
      zres.reason << "zlib:  error: Corrupted compressed file. (" << (int) ch << ")\n"; 
      return ZLIB_FAILURE;
     }

    for (j=0; j!=256; j++) markerlist[j] = nreplaced;
    for (j=0; j!=nreplaced; j++) markerlist[ marker[j] ] = j;


    /* Remove the header. */
    *buffer0_hd = bufpt;

   /* Replace chars. */
   if (nreplaced>0)
    while (bufpt!=0)
    {
     if (bufpt->ch == forcingchar)
      {
	bufpt->ch = bufpt->nxt->ch;	/* Remove the forcing char. */
	oldptr = bufpt->nxt;   bufpt->nxt = bufpt->nxt->nxt;   sczfree(oldptr);
	bufpt = bufpt->nxt;
      }
     else
      { /* Check for match to marker characters. */
       j = markerlist[ bufpt->ch ];
       if (j<nreplaced)
        {	/* If match, insert the phrase. */
  	  bufpt->ch = phrase[j][0];
          tmpbuf = new_scz_item();
          tmpbuf->ch = phrase[j][1];
          tmpbuf->nxt = bufpt->nxt;
          bufpt->nxt = tmpbuf;
          bufpt = tmpbuf->nxt;
        }
       else bufpt = bufpt->nxt;
      }
    }
  } /*iter*/
 return 1;
}



/************************************************************************/
/* Scz_Decompress_File - Decompresses input file to output file.        */
/*  First argument is input file name.  Second argument is output file  */
/*  name.  The two file names must be different.                        */
/*                                                                      */
/************************************************************************/
 int Scz_Decompress_File( char *infilename, char *outfilename )
 {
 int k, success, sz1=0, sz2=0, buflen, continuation;
 unsigned char ch, chksum, chksum0;
 struct scz_item *buffer0_hd, *buffer0_tl, *bufpt, *bufprv;
 FILE *infile=0, *outfile;
 DECOMPRESS_WATCHDOG=0;

 infile = fopen(infilename,"rb");
 if (infile==0) {return 1;} //

 outfile = fopen(outfilename,"wb");
 if (outfile==0) {return 2;} // ERROR: Cannot open output file

 do 
  { /*Segment*/

   /* Read file segment into linked list for expansion. */
   /* Get the 6-byte header to find the number of chars to read in this segment. */
   /*  (magic number (101), magic number (98), iter-count, seg-size (MBS), seg-size, seg-size (LSB). */ 
   buffer0_hd = 0;  buffer0_tl = 0;  bufprv = 0;

   ch = getc(infile);   sz1++;		/* Byte 1, expect magic numeral 101. */
   if (feof(infile)){ zres.reason << "zlib:  error: Premature eof\n"; return ZLIB_FAILURE; }
   if (ch!=101) { zres.reason << "zlib:  error: This does not look like a compressed file. (" << (int) ch << ")\n"; return ZLIB_FAILURE;}
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = getc(infile);  sz1++;		/* Byte 2, expect magic numeral 98. */
   if ((feof(infile)) || (ch!=98)) { 
    zres.reason << "zlib:  error: This does not look like a compressed file. (" << (int) ch << ")\n"; return ZLIB_FAILURE;
   }
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = getc(infile);  sz1++;		/* Byte 3, iteration-count. */
   if (feof(infile)) { zres.reason << "zlib:  error: This does not look like a compressed file.\n"; return ZLIB_FAILURE; }
   bufprv = buffer0_tl;
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = getc(infile);			/* Byte 4, MSB of segment buffer length. */
   if (feof(infile)) { zres.reason << "zlib:  error: This does not look like a compressed file.\n"; return ZLIB_FAILURE; }
   buflen = ch << 16;
   ch = getc(infile);			/* Byte 5, middle byte of segment buffer length. */
   if (feof(infile)) { zres.reason << "zlib:  error: This does not look like a compressed file.\n"; return ZLIB_FAILURE; }
   buflen = (ch << 8) | buflen;
   ch = getc(infile);			/* Byte 6, LSB of segment buffer length. */
   if (feof(infile)) { zres.reason << "zlib:  error: This does not look like a compressed file.\n"; return ZLIB_FAILURE; }
   buflen = ch | buflen;

   k = 0;
   ch = getc(infile);
   while ((!feof(infile)) && (k<buflen))
    {
     bufprv = buffer0_tl;
     scz_add_item( &buffer0_hd, &buffer0_tl, ch );
     sz1++;  k++;
     ch = getc(infile);
     
//     DECOMPRESS_WATCHDOG+=sz1;
//     if(DECOMPRESS_WATCHDOG >= DECOMPRESSION_OVERLOAD)
//     {
//       zres.reason << "zlib:  error: Decompression overload!\n"; return ZLIB_FAILURE;
//     }
    }

   chksum0 = ch;
   ch = getc(infile);
   // printf("End ch = '%c'\n", ch);
   if (k<buflen) { zres.reason << "zlib:  error: Unexpectedly short file.\n"; }
   /* Decode the 'end-marker'. */
   if (ch==']') continuation = 0;
   else
   if (ch=='[') continuation = 1;
   else { zres.reason << "zlib:  error: Reading compressed file. (" << (int) ch << ")\n";  return ZLIB_FAILURE; }

   success = Scz_Decompress_Seg( &buffer0_hd );	/* Decompress the buffer !!! */
   if (!success) return ZLIB_FAILURE;

   /* Write the decompressed file out. */
   chksum = 0;
   bufpt = buffer0_hd;
   while (bufpt!=0)
    {
     fputc( bufpt->ch, outfile );
     sz2++;   chksum += bufpt->ch;
     bufpt = bufpt->nxt;
    }
   if (chksum != chksum0) { zres._warnings_ << "zlib:  warning: Checksum mismatch (" << (long) chksum << "vs" << (long) chksum0 << ")\n"; }

  } /*Segment*/
 while (continuation);

 fclose(infile);
 fclose(outfile);
// printf("Decompression ratio = %g\n", (float)sz2 / (float)sz1 );
 zres.decompressionRatio = (float)sz2 / (float)sz1;
 return 1;
}



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
 int Scz_Decompress_File2Buffer( char *infilename, stringstream &__outbuf__ )
 {
 long k, success, sz1=0, sz2=0, buflen, continuation, totalin=0, totalout=0;
 unsigned char ch, chksum, chksum0;
 struct scz_item *buffer0_hd, *buffer0_tl, *bufpt, *bufprv, *sumlst_hd=0, *sumlst_tl=0;
 FILE *infile=0;
 DECOMPRESS_WATCHDOG=0;

 infile = fopen(infilename,"rb");
 if (infile==0) { zres.reason << "zlib:  error: Cannot open input file '" << infilename << "'.\n";  return ZLIB_FAILURE; }

 do 
  { /*Segment*/

   /* Read file segment into linked list for expansion. */
   /* Get the 6-byte header to find the number of chars to read in this segment. */
   /*  (magic number (101), magic number (98), iter-count, seg-size (MBS), seg-size, seg-size (LSB). */ 
   buffer0_hd = 0;  buffer0_tl = 0;

   ch = getc(infile);   sz1++;		/* Byte 1, expect magic numeral 101. */
   if ((feof(infile)) || (ch!=101)) { zres.reason << "zlib:  error: This does not look like a compressed file.\n"; return ZLIB_FAILURE; }
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = getc(infile);  sz1++;		/* Byte 2, expect magic numeral 98. */
   if ((feof(infile)) || (ch!=98)) { 
     zres.reason << "zlib:  error: This does not look like a compressed file. (" << (long) ch << ")\n"; return ZLIB_FAILURE; 
   }
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = getc(infile);  sz1++;		/* Byte 3, iteration-count. */
   if (feof(infile)) { zres.reason << "zlib:  error: This does not look like a compressed file.\n"; return ZLIB_FAILURE; }
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = getc(infile);			/* Byte 4, MSB of segment buffer length. */
   if (feof(infile)) { zres.reason << "zlib:  error: This does not look like a compressed file.\n"; return ZLIB_FAILURE; }
   buflen = ch << 16;
   ch = getc(infile);			/* Byte 5, middle byte of segment buffer length. */
   if (feof(infile)) { zres.reason << "zlib:  error: This does not look like a compressed file.\n"; return ZLIB_FAILURE; }
   buflen = (ch << 8) | buflen;
   ch = getc(infile);			/* Byte 6, LSB of segment buffer length. */
   if (feof(infile)) { zres.reason << "zlib:  error: This does not look like a compressed file.\n"; return ZLIB_FAILURE; }
   buflen = ch | buflen;
 
   k = 0;
   ch = getc(infile);
   while ((!feof(infile)) && (k<buflen))
    {
     scz_add_item( &buffer0_hd, &buffer0_tl, ch );
     sz1++;  k++;
     ch = getc(infile);
     
//     DECOMPRESS_WATCHDOG+=sz1;
//     if(DECOMPRESS_WATCHDOG >= DECOMPRESSION_OVERLOAD)
//     {
//       zres.reason << "zlib:  error: Decompression overload!\n"; return ZLIB_FAILURE;
//     }
    }

   chksum0 = ch;
   ch = getc(infile);
   // printf("End ch = '%c'\n", ch);
 
   if (k<buflen) { zres._warnings_ << "zlib:  warning: Unexpectedly short file.\n"; }
   totalin = totalin + sz1 + 4;		/* (+4, because chksum+3buflen chars not counted above.) */
   DECOMPRESS_WATCHDOG+=totalin;
   
   /* Decode the 'end-marker'. */
   if (ch==']') continuation = 0;
   else
   if (ch=='[') continuation = 1;
   else { zres.reason << "zlib:  error: Reading compressed file. (" << (int) ch << ")\n";  return ZLIB_FAILURE; }

   success = Scz_Decompress_Seg( &buffer0_hd );	/* Decompress the buffer !!! */
   if (!success) return ZLIB_FAILURE;

   /* Check checksum and sum length. */
   sz2 = 0;       /* Get buffer size. */
   bufpt = buffer0_hd;
   chksum = 0;
   bufprv = 0;
   while (bufpt!=0)
    {
     chksum += bufpt->ch;
     sz2++;  bufprv = bufpt;
     bufpt = bufpt->nxt;
    }
   if (chksum != chksum0) { zres._warnings_ << "zlib:  warning: Checksum mismatch (" << chksum << "vs" << chksum0 << ")\n"; }

   /* Attach to tail of main list. */
   totalout = totalout + sz2;
   if (sumlst_hd==0) sumlst_hd = buffer0_hd;
   else sumlst_tl->nxt = buffer0_hd;
   sumlst_tl = bufprv;

  } /*Segment*/
 while (continuation);

 sz2 = 0;
 while (sumlst_hd!=0)
  {
   bufpt = sumlst_hd;
   __outbuf__ << bufpt->ch;
   sumlst_hd = bufpt->nxt;
   sczfree(bufpt);
  }
 if (sz2 > totalout) zres._warnings_ << "zlib:  warning: Unexpected overrun error\n";

 fclose(infile);
// printf("Decompression ratio = %g\n", (float)totalout / (float)totalin );
 zres.decompressionRatio = (float)totalout / (float)totalin;
 return 1;
}


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
 int Scz_Decompress_Buffer2Buffer( char *inbuffer, int N, stringstream &__outbuf__ )
 {
 long k, success, sz1=0, sz2=0, buflen, continuation, totalin=0, totalout=0;
 unsigned char ch, chksum, chksum0;
 struct scz_item *buffer0_hd, *buffer0_tl, *bufpt, *bufprv, *sumlst_hd=0, *sumlst_tl=0;
 DECOMPRESS_WATCHDOG=0;

 do 
  { /*Segment*/

   /* Convert buffer segment into linked list for expansion. */
   /* Get the 6-byte header to find the number of chars to read in this segment. */
   /*  (magic number (101), magic number (98), iter-count, seg-size (MBS), seg-size, seg-size (LSB). */ 
   buffer0_hd = 0;  buffer0_tl = 0;
   totalin = totalin + N;

   ch = inbuffer[sz1++];		/* Byte 1, expect magic numeral 101. */
   if ((sz1>8) || (ch!=101)) { zres.reason << "zlib:  error: This does not look like a compressed buffer.\n"; return ZLIB_FAILURE;}
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = inbuffer[sz1++];		/* Byte 2, expect magic numeral 98. */
   if (ch!=98) { zres.reason << "zlib:  error: This does not look like a compressed buffer. (" << (int) ch << ")\n"; return ZLIB_FAILURE; }
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = inbuffer[sz1++];		/* Byte 3, iteration-count. */
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = inbuffer[sz1++];			/* Byte 4, MSB of segment buffer length. */
   buflen = ch << 16;
   ch = inbuffer[sz1++];			/* Byte 5, middle byte of segment buffer length. */
   buflen = (ch << 8) | buflen;
   ch = inbuffer[sz1++];			/* Byte 6, LSB of segment buffer length. */
   buflen = ch | buflen;
 
   k = 0;
   ch = inbuffer[sz1++];
   if (N<buflen+sz1+1) { zres._warnings_ << "zlib:  warning: Unexpectedly short buffer.\n";  buflen = N - sz1 - 1; }
   while (k<buflen)
    {
     scz_add_item( &buffer0_hd, &buffer0_tl, ch );
     ch = inbuffer[sz1++];     k++;
     
//     DECOMPRESS_WATCHDOG+=sz1;
//     if(DECOMPRESS_WATCHDOG >= DECOMPRESSION_OVERLOAD)
//     {
//       zres.reason << "zlib:  error: Decompression overload!\n"; return ZLIB_FAILURE;
//     }
    }

   chksum0 = ch;
   ch = inbuffer[sz1++];
   // printf("End ch = '%c'\n", ch);

   /* Decode the 'end-marker'. */
   if (ch==']') continuation = 0;
   else if (ch=='[') continuation = 1;
   else { zres.reason << "zlib:  error: Reading compressed buffer. (" << (int) ch << ")\n";  return ZLIB_FAILURE; }

   success = Scz_Decompress_Seg( &buffer0_hd );	/* Decompress the buffer !!! */
   if (!success) return ZLIB_FAILURE;

   /* Check checksum and sum length. */
   sz2 = 0;       /* Get buffer size. */
   bufpt = buffer0_hd;
   chksum = 0;
   bufprv = 0;
   while (bufpt!=0)
    {
     chksum += bufpt->ch;
     sz2++;  bufprv = bufpt;
     bufpt = bufpt->nxt;
    }
   if (chksum != chksum0) { zres._warnings_ << "zlib:  warning: Checksum mismatch (" << chksum << "vs" << chksum0 << ")\n"; }

   /* Attach to tail of main list. */
   totalout = totalout + sz2;
   if (sumlst_hd==0) sumlst_hd = buffer0_hd;
   else sumlst_tl->nxt = buffer0_hd;
   sumlst_tl = bufprv;

  } /*Segment*/
 while (continuation);

 sz2 = 0;
 while (sumlst_hd!=0)
  {
   bufpt = sumlst_hd;
   __outbuf__ << bufpt->ch;
   sumlst_hd = bufpt->nxt;
   sczfree(bufpt);
  }
 if (sz2 > totalout) zres._warnings_ << "zlib:  warning: Unexpected overrun error\n";

 //printf("Decompression ratio = %g\n", (float)totalout / (float)totalin );
 zres.decompressionRatio = (float)totalout / (float)totalin;
 return 1;
}
