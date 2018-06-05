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
 #include "zstream.h"
 #include "zcompress.h"
 #include "zdecompress.h"



/************************************************************************/
/* Scz_File_Open - Open a file for reading or writing compressed data.	*/
/*  First argument is file-name.  Second argument is 'r' or 'w'.	*/
/*  Returns 0 on failure to open file.					*/
/************************************************************************/
 struct SCZ_File *Scz_File_Open( char *filename, char *mode )
 {
 struct SCZ_File *stateptr;

 stateptr = (struct SCZ_File *)malloc(sizeof(struct SCZ_File));
 if (mode[0]=='r')
  {
   stateptr->mode = 'r';
   stateptr->fptr = fopen(filename,"rb");
  }
 else if (mode[0]=='w')
  {
   stateptr->mode = 'w';
   stateptr->fptr = fopen(filename,"wb");
  }
 else {printf("SCZ ERROR: Bad file open mode '%s'\n", mode);  return 0;}
 if (stateptr->fptr==0) {printf("SCZ ERROR: Cannot open output file '%s'\n", filename); return 0;}
 stateptr->buffer_hd = 0;
 stateptr->buffer_tl = 0;
 stateptr->rdptr = 0;
 stateptr->bufsz = 0;
 stateptr->totalsz = 0;
 stateptr->chksum = 0;
 stateptr->eof = 0;
 stateptr->feof = 0;
 return stateptr;
}


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
 int Scz_WriteString( struct SCZ_File *sczfile, unsigned char *buffer, int N )
 {
 struct scz_item *bufpt;
 int j=0;

 if (N>=SCZ_MAX_BUF) {printf("SCZ Error: Buffer length too large.\n"); return 0;}
 if ((sczfile==0) || (sczfile->fptr==0) || (sczfile->mode!='w'))
  {printf("SCZ Error: File not open for writing.\n"); return 0;}

 /* Add buffer to linked list. */
 while (j < N)
  {
   scz_add_item( &(sczfile->buffer_hd), &(sczfile->buffer_tl), buffer[j] );
   sczfile->chksum += buffer[j];  
   sczfile->bufsz++;  j++;  
  }

 if (sczfile->bufsz > sczbuflen/2)	/* Compress out, if enough accumulated. */
  {
   if (sczfile->totalsz > 0) fprintf(sczfile->fptr,"[");	/* Place continuation marker. */

   Scz_Compress_Seg( &(sczfile->buffer_hd), sczfile->bufsz );

   /* Write the file out. */
   while (sczfile->buffer_hd!=0)
    {
     fputc( sczfile->buffer_hd->ch, sczfile->fptr );
     bufpt = sczfile->buffer_hd;
     sczfile->buffer_hd = sczfile->buffer_hd->nxt;
     sczfree(bufpt);
    }
   sczfile->buffer_tl = 0;
   sczfile->totalsz = sczfile->totalsz + sczfile->bufsz;
   sczfile->bufsz = 0;
   fprintf(sczfile->fptr,"%c", sczfile->chksum); 
   sczfile->chksum = 0;
  }
 return 1;
}


/************************************************************************/
/* Scz_File_Close - Close a file after reading or writing compressed 	*/
/*  data.  Argument is scz_file to be closed.				*/
/************************************************************************/
 void Scz_File_Close( struct SCZ_File *sczfile )
 {
 struct scz_item *bufpt;

 if ((sczfile==0) || (sczfile->fptr==0)) {printf("SCZ Error: File not open for writing.\n");}
 if (sczfile->mode=='w')
  {
   if (sczfile->bufsz > 0) 	/* Flush the scz buffer. */
    {
     if (sczfile->totalsz > 0) fprintf(sczfile->fptr,"[");	/* Place continuation marker. */

     Scz_Compress_Seg( &(sczfile->buffer_hd), sczfile->bufsz );

     /* Write the file out. */
     while (sczfile->buffer_hd!=0)
      {
       fputc( sczfile->buffer_hd->ch, sczfile->fptr );
       bufpt = sczfile->buffer_hd;
       sczfile->buffer_hd = sczfile->buffer_hd->nxt;
       sczfree(bufpt);
      }
     sczfile->buffer_tl = 0;
     sczfile->totalsz = sczfile->totalsz + sczfile->bufsz;
     sczfile->bufsz = 0;
     fprintf(sczfile->fptr,"%c", sczfile->chksum); 
    }
   fprintf(sczfile->fptr,"]");	/* Place no-continuation marker. */
  }

 fclose(sczfile->fptr);
 sczfile->fptr = 0;
 free(sczfile);
}



/************************************************************************/
/* Scz_Feof - Test for end of a compressed input file.			*/
/************************************************************************/
 int Scz_Feof( struct SCZ_File *sczfile )
 { return sczfile->eof; }






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
 int Scz_ReadString( struct SCZ_File *sczfile, char *delimiter, unsigned char *outbuffer, int maxN )
 {
 int j, k, success, sz2=0, buflen, inquote=0, stop;
 unsigned char ch, chksum, chksum0;
 struct scz_item *bufpt, *bufprv, *buffer0_hd, *buffer0_tl;

 if ((sczfile==0) || (sczfile->fptr==0) || (sczfile->mode!='r'))
  {printf("SCZ Error: File not open for reading.\n"); return 0;}

 if ((sczfile->bufsz < maxN) && (! sczfile->feof))
  { /*Unpack the next Segment*/
   buffer0_hd = 0;  buffer0_tl = 0;
   ch = getc(sczfile->fptr);   	/* Byte 1, expect magic numeral 101. */
   if ((feof(sczfile->fptr)) || (ch!=101)) {printf("Error1: This does not look like a compressed file.\n"); return 0;}
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );
   ch = getc(sczfile->fptr);  		/* Byte 2, expect magic numeral 98. */
   if ((feof(sczfile->fptr)) || (ch!=98)) {printf("Error2: This does not look like a compressed file. (%d)\n", ch); return 0;}
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );
   ch = getc(sczfile->fptr);  		/* Byte 3, iteration-count. */
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );
   ch = getc(sczfile->fptr);			/* Byte 4, MSB of segment buffer length. */
   buflen = ch << 16;
   ch = getc(sczfile->fptr);			/* Byte 5, middle byte of segment buffer length. */
   buflen = (ch << 8) | buflen;
   ch = getc(sczfile->fptr);			/* Byte 6, LSB of segment buffer length. */
   if (feof(sczfile->fptr)) {printf("Error3: This does not look like a compressed file.\n"); return 0;}
   buflen = ch | buflen;
   k = 0;
   ch = getc(sczfile->fptr);
   while ((!feof(sczfile->fptr)) && (k<buflen))
    {
     scz_add_item( &buffer0_hd, &buffer0_tl, ch );
     ch = getc(sczfile->fptr);   k++;
    }
   chksum0 = ch;
   ch = getc(sczfile->fptr);
   if (k<buflen) {printf("Error: Unexpectedly short file.\n");}
   /* Decode the 'end-marker'. */
   if (ch==']') sczfile->feof = 1;
   else
   if (ch!='[') {printf("Error4: Reading compressed file. (%d)\n", ch);  return 0; }

   success = Scz_Decompress_Seg( &buffer0_hd );	/* Decompress the buffer !!! */
   if (!success) {printf("Error5: Uncompressing file. \n");  return 0;}

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
   if (chksum != chksum0) {printf("Error: Checksum mismatch (%dvs%d)\n", chksum, chksum0);}
   /* Attach to tail of main list. */
   sczfile->totalsz = sczfile->totalsz + sz2;
   sczfile->bufsz = sczfile->bufsz + sz2;
   if (sczfile->buffer_hd==0) sczfile->buffer_hd = buffer0_hd;
   else sczfile->buffer_tl->nxt = buffer0_hd;
   sczfile->buffer_tl = bufprv;
  } /*Segment*/

 /* Convert line or tag from list to the out-buffer. */
 if (sczfile->buffer_hd==0) sczfile->eof = 1;
 k = 0;  stop = 0;
 while ((sczfile->buffer_hd!=0) && (!stop) && (k<maxN))
  {
   bufpt = sczfile->buffer_hd;
   ch = bufpt->ch;
   if (ch=='"') inquote = ! inquote;
   j = 0;
   while ((delimiter[j]!=ch) && (delimiter[j]!='\0')) j++;
   if ((delimiter[j]=='\0') || inquote || ((ch=='<') && (k==0)))
    {
     outbuffer[k++] = ch;
     sczfile->buffer_hd = bufpt->nxt;
     sczfree(bufpt);
    }
   else
    {
     if (ch=='\n') { stop = 1;  sczfile->buffer_hd = bufpt->nxt;  sczfree(bufpt);  sczfile->bufsz--; }
     else
     if (ch=='>')
      { stop = 1;  outbuffer[k++] = ch;  sczfile->buffer_hd = bufpt->nxt;  sczfree(bufpt); }
     else stop = 1;
    }
  }
 sczfile->bufsz = sczfile->bufsz - k;
 outbuffer[k] = '\0';
 if (sczfile->buffer_hd==0) sczfile->buffer_tl = 0;
 return 1;
}


