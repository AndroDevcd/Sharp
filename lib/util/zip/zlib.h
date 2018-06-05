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
 #ifndef _ZLIB
 #define _ZLIB

#include "../../../stdimports.h"
#include <stdint.h>

using namespace std;
 
 #define ZLIB_NEXT_SEGMENT false
 #define ZLIB_LAST_SEGMENT true
 
 #define ZLIB_FAILURE -(0x503)
 
 /*
 * Zlib response data
 *
 * This data structure holds the response code from the 
 * zlib functions. Use this to check for errors, warnings, 
 * and to get useful information such as the compression size 
 * of your data;
 *
 */
 struct ZLib_Response {
     /* 
     * u2 size_t
     *
     * This will hold the data for the 
     * inital and final compression size 
     *
     * byte1 := the inital compression size
     * byte2 := the final compressed size 
     *
     * The inital and final compressed size is calculated 
     * in bytes 
     *
     */
     int64_t begin_sz;
     int64_t end_sz;

     /* The compression ratio of the data
     *
     * This ratio is a %ratio : 1
     */
     float compressionRatio,
          decompressionRatio;
     
     /* (Unused) */
     int64_t data;// if there is any extra data to be returned
     
     stringstream _warnings_; // our buffer for if there are any warnings
     stringstream reason; // if compression fails, this will be populated with the reason why it failed
     
     /* 
     * Compare this code to the 
     * standard failure code (ZLIB_FAILURE)
     *
     * if (response == ZLIB_FAILURE) // compression failed
     * else // continue
     *
     */
     int response; // the response from our library
 } ;
 
 extern struct ZLib_Response zres;
 
 /*
 * Zip compression library
 *
 * This is the high level portion of the library.
 * We make this class to abstract the library from the 
 * confusing concepts of the library.
 *
 */
 class Zlib {
     
     public:
        static bool AUTO_CLEAN;
        void Compress(string file, string output);
        void Decompress(string file, string output);
        void Compress_Buffer2File(string buffer, string outfile);
        void Compress_Buffer2Buffer(string buffer, stringstream &__obuff__, bool lastseg);
        void Decompress_File2Buffer(string file, stringstream &__buf);
        void Decompress_Buffer2Buffer(string buffer, stringstream &__buf);
        void Cleanup();
 };
 
#endif // _ZLIB 