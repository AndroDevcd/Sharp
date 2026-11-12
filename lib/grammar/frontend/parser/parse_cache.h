//
// Parse cache: the syntax tree of a source file, kept on disk between compiles.
//
// Every compile reads all source files - the program's and the whole standard
// library - and turns each one into tokens and then into a syntax tree. For a
// file that has not changed since the last compile the result is the same tree
// every time. The cache stores that tree; the next compile loads it instead of
// tokenizing and parsing the file again.
//
//      compile 1:   read file -> tokenize -> parse -> tree      (and save the tree)
//      compile 2:   read file -> same contents? -> load the tree
//
// Where:      one file per source file in the cache folder, by default
//             <home>/.sharpc/cache   (-cache <folder> picks another one,
//             -cacheoff turns the cache off).
// Valid when: the 64 bit hash and the length of the source text are the ones
//             the tree was made from, AND the tree was written by this very
//             compiler executable (its size and modification time are part of
//             the entry - a rebuilt compiler never reads trees an older build
//             wrote, so a change to the tree's layout cannot go wrong).
// Not cached: files whose parse reported an error or a warning - those have to
//             be parsed again so the message is printed again.
//
// The source text itself is still read on every compile: it is needed for the
// hash, for the lines shown in error messages and for the copy that goes into
// the executable.
//
// Comparison: this is what a precompiled header is to a C++ compiler, per file
// and without anything to set up.
//

#ifndef SHARP_PARSE_CACHE_H
#define SHARP_PARSE_CACHE_H

#include <string>

class parser;
class tokenizer;

/**
 * Try to get the syntax tree of the file `fileName` with the contents
 * `sourceText` from the cache. On a hit the tokenizer (lines only, no tokens)
 * and the parser (holding the tree) are created and true is returned.
 */
bool parse_cache_load(const std::string &fileName, const std::string &sourceText,
                      tokenizer *&outTokenizer, parser *&outParser);

/** Save the tree `p` just parsed from `sourceText` (does nothing if it cannot be cached). */
void parse_cache_store(const std::string &fileName, const std::string &sourceText, parser *p);

/** For -timings. */
long parse_cache_hits();
long parse_cache_misses();

#endif //SHARP_PARSE_CACHE_H
