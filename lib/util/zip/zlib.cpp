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
 #include "zlib.h"
 #include "zinclude.h"
 #include <stdio.h>
 #include <string.h>
 #include <iostream>
 #include <sstream>
 #include <new>

 using namespace std;
 
 
 ZLib_Response zres;
 bool Zlib::AUTO_CLEAN = false;
 #define nil ((void *)0)
 
 char* c_ptr(string data){
    char *c = new (nothrow) char[data.size() + 1];
    
    if(c == nil){
      char *c2 = new char[1];
      c2[0] = 5;
      return c2;
    }
    
   for(long i = 0; i < data.size(); i++)
      c[i] = data.at(i);
   c[data.size()] = 0;
    return c;
 }
 
 void Zlib::Compress(string file, string output){
     if(AUTO_CLEAN)
       Cleanup();
       
     zres.response = Scz_Compress_File( c_ptr(file), c_ptr(output) );
 }
 
 void Zlib::Decompress(string file, string output){
     if(AUTO_CLEAN)
       Cleanup();
       
     zres.response = Scz_Decompress_File( c_ptr(file), c_ptr(output) );
 }
 
 void Zlib::Compress_Buffer2File(string buffer, string outfile){
     if(AUTO_CLEAN)
       Cleanup();
       
     char* c = c_ptr(buffer);
     unsigned char *cBuffer = (unsigned char*) &c[0];
     zres.response = Scz_Compress_Buffer2File( cBuffer, buffer.size(), c_ptr(outfile) );
 }
 
 void Zlib::Compress_Buffer2Buffer(string buffer, stringstream &__obuff__, bool lastseg){
     if(AUTO_CLEAN)
       Cleanup();
       
     zres.response = Scz_Compress_Buffer2Buffer( c_ptr(buffer), buffer.size(), __obuff__, (int) lastseg );
 }
 
 void Zlib::Decompress_File2Buffer(string file, stringstream &__buf){
     if(AUTO_CLEAN)
       Cleanup();
       
     zres.response = Scz_Decompress_File2Buffer( c_ptr(file), __buf );
 }
 
 void Zlib::Decompress_Buffer2Buffer(string buffer, stringstream &__outbuf_){
     if(AUTO_CLEAN)
       Cleanup();
       
     zres.response = Scz_Decompress_Buffer2Buffer( c_ptr(buffer), buffer.size(), __outbuf_ );
 }
 
 void Zlib::Cleanup(){
     zres.begin_sz=0;
     zres.end_sz=0;
     zres.compressionRatio=0;
     zres.decompressionRatio=0;
     zres._warnings_.str("");
     zres.reason.str("");
     zres.response=0;
     scz_cleanup();
 }
 
 