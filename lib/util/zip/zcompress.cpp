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
#include "../../../stdimports.h"
 
 using namespace std;
 
 int *scz_freq2=0, w=0; 
 extern unsigned long DECOMPRESS_WATCHDOG;
 
/*------------------------------------------------------------*/
/* Add an item to a value-sorted list of maximum-length N.    */
/* Sort from largest to smaller values.  (A descending list.) */
/*------------------------------------------------------------*/
 void scz_add_sorted_nmax( struct scz_amalgam *list, unsigned char *phrase, int lngth, int value, int N )
 {
 int j, k=0, m;

 if (value <= list[N-1].value) return;
 while ((k<N) && (list[k].value >= value)) k++;
 if (k==N) return;
 j = N-2;
 while (j>=k)
  { 
   list[j+1].value = list[j].value;  
   for (m=0; m<lngth; m++) list[j+1].phrase[m] = list[j].phrase[m];  
   j--;
  }
 list[k].value = value;
 for (j=0; j<lngth; j++) list[k].phrase[j] = phrase[j];
}


/*------------------------------------------------------------*/
/* Add an item to a value-sorted list of maximum-length N.    */
/* Sort from smaller to larger values.  (An ascending list.)  */
/*------------------------------------------------------------*/
 void scz_add_sorted_nmin( struct scz_amalgam *list, unsigned char *phrase, int lngth, int value, int N )
 {
 int j, k=0, m;

 while ((k<N) && (list[k].value <= value)) k++;
 if (k==N) return;
 j = N-2;
 while (j>=k) 
  { 
   list[j+1].value = list[j].value;
   for (m=0; m<lngth; m++) list[j+1].phrase[m] = list[j].phrase[m];
   j--;
  }
 list[k].value = value;
 for (j=0; j<lngth; j++) list[k].phrase[j] = phrase[j];
}


/*----------------------------------------------------------------------*/
/* Analyze a buffer to determine the frequency of characters and pairs. */
/*----------------------------------------------------------------------*/
 void scz_analyze( struct scz_item *buffer0_hd, int *freq1, int *freq2 )
 {
 int j, k;
 struct scz_item *ptr;

 memset( freq1, 0, sizeof(int)*256 );
 memset( freq2, 0, sizeof(int)*256*256 );
 ptr = buffer0_hd;
 if (ptr==0) return;
 k = ptr->ch;
 freq1[k]++;
 ptr = ptr->nxt;
 while (ptr!=0)
  {
   j = k;
   k = ptr->ch;
   freq1[k]++;
   freq2[j*256+k]++;
   ptr = ptr->nxt;
  }
}




/*------------------------------------------------------*/
/* Compress a buffer, step.  Called iteratively.	*/
/*------------------------------------------------------*/
 int scz_compress_iter( struct scz_item **buffer0_hd )
 {
 int nreplace=250;
 int freq1[256], markerlist[256];
 int i, j, k, replaced, nreplaced, sz3=0, saved=0, saved_pairfreq[256], saved_charfreq[257];
 struct scz_item *head1, *head2, *bufpt, *tmpbuf, *cntpt;
 unsigned char word[10], forcingchar;
 struct scz_amalgam char_freq[257], phrase_freq_max[256];

 /* Examine the buffer. */
 /* Determine histogram of character usage, and pair-frequencies. */
 if (scz_freq2==0) scz_freq2 = (int *)malloc(256*256*sizeof(int));
 scz_analyze( *buffer0_hd, freq1, scz_freq2 );

 /* Initialize rank vectors. */
 memset( saved_pairfreq, 0, 256 * sizeof(int) );
 memset( saved_charfreq, 0, 256 * sizeof(int) );
 for (k=0; k<256; k++)
  {
   char_freq[k].value = 1073741824;
   phrase_freq_max[k].value = 0;
  }

 /* Sort and rank chars. */
 for (j=0; j!=256; j++) 
  {
   word[0] = j;
   scz_add_sorted_nmin( char_freq, word, 1, freq1[j], nreplace+1 );
  }

 /* Sort and rank pairs. */
 i = 0;  
 for (k=0; k!=256; k++) 
  for (j=0; j!=256; j++) 
   if (scz_freq2[j*256+k]!=0)
    {
     word[0] = j;  word[1] = k;
     scz_add_sorted_nmax( phrase_freq_max, word, 2, scz_freq2[j*256+k], nreplace );
    }

 /* Use the least-used character(s) for special expansion symbol(s). I.E. "markers". */
 /* Now go through, and replace any instance of the max_pairs_phrase's with the respective markers. */
 /* And insert before any natural occurrences of the markers, the forcingchar. */
 /*  These two sets should be mutually exclusive, so it should not matter which order this is done. */

 head1 = 0;	head2 = 0;
 forcingchar = char_freq[0].phrase[0];
 k = -1;  j = 0;
 while ((j<nreplace) && (char_freq[j+1].value < phrase_freq_max[j].value - 3))
  { j++;   k = k + phrase_freq_max[j].value - char_freq[j+1].value; }
 nreplaced = j;

 if (nreplaced == 0) return 0; /* Not able to compress this data further with this method. */

 /* Now go through the buffer, and make the following replacements:
    - If equals forcingchar or any of the other maker-chars, then insert forcing char in front of them.
    - If the next pair match any of the frequent_phrases, then replace the phrase by the corresponding marker.
 */


/* Define a new array, phrase_freq_max2[256], which is a lookup array for the first character of
   a two-character phrase.  Each element points to a list of second characters with "j" indices.
   First chars which have none are null, stopping the search immediately.
*/
memset( sczphrasefreq, 0, 256 * sizeof(struct scz_item2 *) );
for (j=0; j!=256; j++) markerlist[j] = nreplaced+1;
for (j=0; j!=nreplaced+1; j++) markerlist[ char_freq[j].phrase[0] ] = j;
for (j=0; j!=nreplaced; j++)
 if (phrase_freq_max[j].value>0)
  {
   scztmpphrasefreq = new_scz_item2();
   scztmpphrasefreq->ch = phrase_freq_max[j].phrase[1];
   scztmpphrasefreq->j = j;
   k = phrase_freq_max[j].phrase[0];
   scztmpphrasefreq->nxt = sczphrasefreq[k];
   sczphrasefreq[k] = scztmpphrasefreq;
  }


 /* First do a tentative check. */
 bufpt = *buffer0_hd;
 if (nreplaced>0)
 while (bufpt!=0)
  {
   replaced = 0;
   if (bufpt->nxt!=0)
    {  /* Check for match to frequent phrases. */
     k = bufpt->ch;
     if (sczphrasefreq[k]==0) j = nreplaced;
     else
      { unsigned char tmpch;
	tmpch = bufpt->nxt->ch; 
	scztmpphrasefreq = sczphrasefreq[k];
	while ((scztmpphrasefreq!=0) && (scztmpphrasefreq->ch != tmpch)) scztmpphrasefreq = scztmpphrasefreq->nxt;
        if (scztmpphrasefreq==0) j = nreplaced; else j = scztmpphrasefreq->j;
      }

     if (j<nreplaced)
      { /* If match, the phrase would be replaced with corresponding marker. */
	saved++;
	saved_pairfreq[j]++;		/* Keep track of how many times this phrase occured. */
	bufpt = bufpt->nxt->nxt;	/* Skip over. */
	replaced = 1;
      }
    }
   if (!replaced)
    {  /* Check for match of marker characters. */
     j = markerlist[ bufpt->ch ];
     if (j<=nreplaced)
      {	/* If match, insert forcing character. */
	saved--;
	saved_charfreq[j]--;		/* Keep track of how many 'collisions' this marker-char caused. */
      }
     bufpt = bufpt->nxt;
    }
  }

 // printf("%d saved\n", saved);
 if (saved<=1) return 0; /* Not able to compress this data further with this method. Buffer unchanged. */

 /* Now we know which marker/phrase combinations do not actually pay. */
 /* Reformulate the marker list with reduced set. */
 /* The least frequent chars become the forcing char and the marker characters. */
 /* Store out the forcing-char, markers and replacement phrases after a magic number. */
 scz_add_item( &head1, &head2, char_freq[0].phrase[0] );	/* First add forcing-marker (escape-like) value. */
 scz_add_item( &head1, &head2, 0 );	/* Next, leave place-holder for header-symbol-count. */
 cntpt = head2;
 k = 0;  saved = 0;
 for (j=0; j<nreplaced; j++)
  if (saved_pairfreq[j] + saved_charfreq[j+1] > 3)
   { unsigned char ch;
    saved = saved + saved_pairfreq[j] + saved_charfreq[j+1] - 3;
    ch = char_freq[j+1].phrase[0];
    scz_add_item( &head1, &head2, ch );  /* Add phrase-marker. */
    char_freq[k+1].phrase[0] = ch;

    ch = phrase_freq_max[j].phrase[0];
    scz_add_item( &head1, &head2, ch );	 /* Add phrase 1st char. */
    phrase_freq_max[k].phrase[0] = ch;

    ch = phrase_freq_max[j].phrase[1];
    scz_add_item( &head1, &head2, ch );	 /* Add phrase 2nd char. */
    phrase_freq_max[k].phrase[1] = ch;
    k++;
   }
 saved = saved + saved_charfreq[0];
 if ((k == 0) || (saved < 6)) 
  { /* Not able to compress this data further with this method. Leave buffer basically unchanged. */
   /* Free the old lists. */
   for (j=0; j!=256; j++) 
    while (sczphrasefreq[j] != 0)
     { scztmpphrasefreq = sczphrasefreq[j]; sczphrasefreq[j] = sczphrasefreq[j]->nxt; sczfree2(scztmpphrasefreq); }

   /* Free the temporarily added items.        */   /* ****   DWS, 05Sep2006, Added to fix memory leak.       */
   while (head1 != 0)
    {
     bufpt = head1->nxt;
     sczfree(head1);
     head1 = bufpt;
    }

   return 0; 
  }

 sz3 = 3 * (k + 1);
 cntpt->ch = k;		/* Place the header-symbol-count. */
 nreplaced = k;
 scz_add_item( &head1, &head2, 91 );	/* Magic barrier. */

 /* Update the phrase_freq tree. */
 /* First free the old list. */
 for (j=0; j!=256; j++) 
  while (sczphrasefreq[j] != 0)
   { scztmpphrasefreq = sczphrasefreq[j]; sczphrasefreq[j] = sczphrasefreq[j]->nxt; sczfree2(scztmpphrasefreq); }
 /* Now make the new list. */
 for (j=0; j!=nreplaced; j++)
  if (phrase_freq_max[j].value>0)
   {
    scztmpphrasefreq = new_scz_item2();
    scztmpphrasefreq->ch = phrase_freq_max[j].phrase[1];
    scztmpphrasefreq->j = j;
    k = phrase_freq_max[j].phrase[0];
    scztmpphrasefreq->nxt = sczphrasefreq[k];
    sczphrasefreq[k] = scztmpphrasefreq;
   }

for (j=0; j!=256; j++) markerlist[j] = nreplaced+1;
for (j=0; j!=nreplaced+1; j++) markerlist[ char_freq[j].phrase[0] ] = j;
// printf("Replacing %d\n", nreplaced);

 bufpt = *buffer0_hd;
 while (bufpt!=0)
  {
   sz3++;
   replaced = 0;
   if (bufpt->nxt!=0)
    {  /* Check for match to frequent phrases. */
     k = bufpt->ch;
     if (sczphrasefreq[k]==0) j = nreplaced;
     else
      { unsigned char tmpch;
	tmpch = bufpt->nxt->ch; 
	scztmpphrasefreq = sczphrasefreq[k];
	while ((scztmpphrasefreq!=0) && (scztmpphrasefreq->ch != tmpch)) scztmpphrasefreq = scztmpphrasefreq->nxt;
        if (scztmpphrasefreq==0) j = nreplaced; else j = scztmpphrasefreq->j;
      }

     if (j<nreplaced)
      { /* If match, replace phrase with corresponding marker. */
	bufpt->ch = char_freq[j+1].phrase[0];
	tmpbuf = bufpt->nxt;
	bufpt->nxt = tmpbuf->nxt;
	sczfree( tmpbuf );
	replaced = 1;
      }
    }
   if (!replaced)
    {  /* Check for match of marker characters. */
     j = markerlist[ bufpt->ch ];
     if (j<=nreplaced)
      {	/* If match, insert forcing character. */
       tmpbuf = new_scz_item();
       tmpbuf->ch = bufpt->ch;
       tmpbuf->nxt = bufpt->nxt;
       bufpt->nxt = tmpbuf;
       bufpt->ch = forcingchar;
       bufpt = tmpbuf;
       sz3 = sz3 + 1;
      }
    }

   bufpt = bufpt->nxt;
  }

 /* Now attach header to main buffer. */
 head2->nxt = *buffer0_hd;
 *buffer0_hd = head1;

 /* Free the old list. */
 for (j=0; j!=256; j++) 
  while (sczphrasefreq[j] != 0)
   { scztmpphrasefreq = sczphrasefreq[j]; sczphrasefreq[j] = sczphrasefreq[j]->nxt; sczfree2(scztmpphrasefreq); }

 return sz3;
}





/*******************************************************************/
/* Scz_Compress - Compress a buffer.  Entry-point to Scz_Compress. */
/*  Compresses the buffer passed in.				   */
/* Pass-in the buffer and its initial size, as sz1. 		   */
/* Returns 1 on success, 0 on failure.				   */
/*******************************************************************/
 int Scz_Compress_Seg( struct scz_item **buffer0_hd, int sz1 )
 {
 struct scz_item *tmpbuf_hd=0, *tmpbuf_tl=0;
 int sz2, iter=0;

 /* Compress. */
 sz2 = scz_compress_iter( buffer0_hd );
 while ((sz2!=0) && (iter<255))
  {
   sz1 = sz2;
   sz2 = scz_compress_iter( buffer0_hd );
   iter++;  
  }

 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, 101 );   /* Place magic start-number(s). */
 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, 98 );
 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, iter );  /* Place compression count. */
 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, sz1>>16 );         /* Place size count (MSB). */
 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, (sz1>>8) & 255 );  /* Place size count. */
 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, sz1 & 255 );       /* Place size count (LSB). */

 tmpbuf_tl->nxt = *buffer0_hd;
 *buffer0_hd = tmpbuf_hd;
 return 1;
}



 long Scz_get_file_length( FILE *fp )
 {
  long len, cur;
  cur = ftell( fp );            /* remember where we are */
  fseek( fp, 0L, SEEK_END );    /* move to the end */
  len = ftell( fp );            /* get position there */
  fseek( fp, cur, SEEK_SET );   /* return back to original position */
  return( len );
}


/************************************************************************/
/* Scz_Compress_File - Compresses input file to output file.		*/
/*  First argument is input file name.  Second argument is output file	*/
/*  name.  The two file names must be different.			*/
/*									*/
/************************************************************************/
 int Scz_Compress_File( char *infilename, char *outfilename )
 {
 struct scz_item *buffer0_hd=0, *buffer0_tl=0, *bufpt;
 int sz1=0, sz2=0, szi, success=1, flen, buflen;
 unsigned char ch, chksum;
 FILE *infile=0, *outfile=0;
 DECOMPRESS_WATCHDOG=0;

 infile = fopen(infilename,"rb");
 if (infile==0) {
  zres.reason << "zlib:  error: Cannot open input file '" << infilename << "'.\n";
  return ZLIB_FAILURE;
 }

 outfile = fopen(outfilename,"wb");
 if (outfile==0) {
   zres.reason << "zlib:  error: Cannot open output file '" << outfilename << "' for writing.\n";
  return ZLIB_FAILURE;
 }

 flen = Scz_get_file_length( infile );
 buflen = flen / sczbuflen + 1;
 buflen = flen / buflen + 1;
 
 DECOMPRESS_WATCHDOG+=buflen;
 if(w==0 && ((DECOMPRESS_WATCHDOG >= DECOMPRESSION_OVERLOAD) || (DECOMPRESS_WATCHDOG >= 
                           (DECOMPRESSION_OVERLOAD - (DECOMPRESSION_OVERLOAD/2))))) // are we close to a compression overload
 {
   w = 1;
   zres._warnings_ << "zlib:  warning: Possible Decompression overload!\n";
 }
 
 if (buflen>=SCZ_MAX_BUF) {
   zres.reason << "zlib:  error: Buffer length too large.\n";
  return ZLIB_FAILURE;
 }

 /* Read file into linked list. */
 ch = getc(infile);
 while (!feof(infile))
  { /*outerloop*/

    chksum = 0;  szi = 0;
    buffer0_hd = 0;   buffer0_tl = 0;
    while ((!feof(infile)) && (szi < buflen))
     {
      scz_add_item( &buffer0_hd, &buffer0_tl, ch );
      sz1++;  szi++;  chksum += ch;  
      ch = getc(infile);
     }

   success = success & Scz_Compress_Seg( &buffer0_hd, szi );

   /* Write the file out. */
   while (buffer0_hd!=0)
    {
     fputc( buffer0_hd->ch, outfile );
     sz2++;
     bufpt = buffer0_hd;
     buffer0_hd = buffer0_hd->nxt;
     sczfree(bufpt);
    }
   fprintf(outfile,"%c", chksum); 
   sz2++;
   if (feof(infile)) fprintf(outfile,"]");	/* Place no-continuation marker. */
   else fprintf(outfile,"[");			/* Place continuation marker. */
   sz2++;

  } /*outerloop*/
 fclose(infile);
 fclose(outfile);

// printf("Initial size = %d,  Final size = %d\n", sz1, sz2);
// printf("Compression ratio = %g : 1\n", (float)sz1 / (float)sz2 );
 zres.begin_sz = sz1; // inital size
 zres.end_sz = sz2; // final size
 zres.compressionRatio = (float)sz1 / (float)sz2;
 free(scz_freq2);
 scz_freq2 = 0;
 return success;
}




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
 int Scz_Compress_Buffer2File( unsigned char *buffer, int N, char *outfilename )
 {
 struct scz_item *buffer0_hd=0, *buffer0_tl=0, *bufpt;
 int sz1=0, sz2=0, szi, success=1, buflen;
 unsigned char chksum;
 FILE *outfile=0;
 DECOMPRESS_WATCHDOG=0;

 outfile = fopen(outfilename,"wb");
 if (outfile==0) { zres.reason << "zlib:  error: Cannot open output file '" << outfilename << "' for writing.\n"; return ZLIB_FAILURE; }

 buflen = N / sczbuflen + 1;
 buflen = N / buflen + 1;
 
 DECOMPRESS_WATCHDOG+=buflen;
 if(w==0 && ((DECOMPRESS_WATCHDOG >= DECOMPRESSION_OVERLOAD) || (DECOMPRESS_WATCHDOG >= 
                           (DECOMPRESSION_OVERLOAD - (DECOMPRESSION_OVERLOAD/2))))) // are we close to a compression overload
 {
   w =1;
   zres._warnings_ << "zlib:  warning: Possible Decompression overload!\n";
 }
 
 if (buflen>=SCZ_MAX_BUF) { zres.reason << "zlib:  error: Buffer length too large.\n"; return ZLIB_FAILURE; }

 while (sz1 < N)
  { /*outerloop*/

    chksum = 0;  szi = 0;
    if (N-sz1 < buflen) buflen = N-sz1;

    /* Convert buffer into linked list. */
    buffer0_hd = 0;   buffer0_tl = 0;
    while (szi < buflen)
     {
      scz_add_item( &buffer0_hd, &buffer0_tl, buffer[sz1] );
      chksum += buffer[sz1];  
      sz1++;  szi++;  
     }

   success = success & Scz_Compress_Seg( &buffer0_hd, szi );

   /* Write the file out. */
   while (buffer0_hd!=0)
    {
     fputc( buffer0_hd->ch, outfile );
     sz2++;
     bufpt = buffer0_hd;
     buffer0_hd = buffer0_hd->nxt;
     sczfree(bufpt);
    }
   fprintf(outfile,"%c", chksum); 
   sz2++;
   if (sz1 >= N) fprintf(outfile,"]");	/* Place no-continuation marker. */
   else fprintf(outfile,"[");		/* Place continuation marker. */
   sz2++;

  } /*outerloop*/
 fclose(outfile);

// printf("Initial size = %d,  Final size = %d\n", sz1, sz2);
// printf("Compression ratio = %g : 1\n", (float)sz1 / (float)sz2 );
 zres.begin_sz = sz1; // inital size
 zres.end_sz = sz2; // final size
 zres.compressionRatio = (float)sz1 / (float)sz2;
 free(scz_freq2);
 scz_freq2 = 0;
 return success;
}



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
 int Scz_Compress_Buffer2Buffer( char *inbuffer, int N, stringstream &__outbuf__, int lastbuf_flag )
 {
 struct scz_item *buffer0_hd=0, *buffer0_tl=0, *bufpt;
 int sz1=0, sz2=0, szi, success=1, buflen;
 unsigned char chksum;
 if(lastbuf_flag)
    DECOMPRESS_WATCHDOG=0;

 buflen = N;
 
 DECOMPRESS_WATCHDOG+=buflen;
 if(w==0 && ((DECOMPRESS_WATCHDOG >= DECOMPRESSION_OVERLOAD) || (DECOMPRESS_WATCHDOG >= 
                           (DECOMPRESSION_OVERLOAD - (DECOMPRESSION_OVERLOAD/2))))) // are we close to a compression overload
 {
   w =1;
   zres._warnings_ << "zlib:  warning: Possible Decompression overload!\n";
 }
 
 if (buflen>=SCZ_MAX_BUF) { zres.reason << "zlib:  error: Buffer length too large.\n"; return ZLIB_FAILURE;}

 chksum = 0;  szi = 0;
 if (N-szi < buflen) buflen = N-szi;
 /* Convert buffer into linked list. */
 buffer0_hd = 0;   buffer0_tl = 0;
 while (szi < buflen)
  {
   scz_add_item( &buffer0_hd, &buffer0_tl, inbuffer[sz1] );
   chksum += inbuffer[sz1];  
   sz1++;  szi++;
  }

 success = success & Scz_Compress_Seg( &buffer0_hd, szi );

 /* Convert list into buffer. */
 sz2 = 0;	/* Get buffer size. */
 bufpt = buffer0_hd;
 while (bufpt!=0) { sz2++;  bufpt = bufpt->nxt;  }
 while (buffer0_hd!=0)
  {
   __outbuf__ << buffer0_hd->ch;
   bufpt = buffer0_hd;
   buffer0_hd = buffer0_hd->nxt;
   sczfree(bufpt);
  }
  
 __outbuf__ << chksum; 
 if (lastbuf_flag) __outbuf__ << ']';	/* Place no-continuation marker. */
 else __outbuf__ << '[';			/* Place continuation marker. */

 // printf("Initial size = %d,  Final size = %d\n", sz1, sz2);
 // printf("Compression ratio = %g : 1\n", (float)sz1 / (float)sz2 );
  zres.begin_sz = sz1; // inital size
  zres.end_sz = sz2; // final size
 zres.compressionRatio = (float)sz1 / (float)sz2;
 free(scz_freq2);
 scz_freq2 = 0;
 return success;
}

 