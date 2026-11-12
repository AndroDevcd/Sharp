//
// Parse cache - see parse_cache.h for what it is and when an entry is valid.
//
// Layout of a cache file (all numbers little endian, as the machine writes them):
//
//      8 bytes   "SHARPAST"
//      u32       format version (PARSE_CACHE_FORMAT)
//      u64       stamp of the compiler executable that wrote it
//      u64       length of the source text
//      u64       hash of the source text (FNV-1a)
//      string    full name of the source file (two files could share a cache
//                name if their path hashes collide; this tells them apart)
//      u32       number of top level nodes
//      nodes     each:  u16 type, i32 line, i32 col,
//                       u32 token count,  tokens (u16 id, u16 type, i32 line, i32 col, string text)
//                       u32 child count,  children (nodes)
//
//      string =  u32 length + the bytes
//

#include "parse_cache.h"
#include "Parser.h"
#include "../tokenizer/tokenizer.h"
#include "../../settings/settings.h"
#include "../../../util/File.h"
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#define PARSE_CACHE_FORMAT 1

static std::atomic<long> cacheHits(0);
static std::atomic<long> cacheMisses(0);

long parse_cache_hits() { return cacheHits; }
long parse_cache_misses() { return cacheMisses; }

// ---------------------------------------------------------------------------
// small helpers
// ---------------------------------------------------------------------------

/** FNV-1a, 64 bit: a fast hash that is plenty to tell "same text" from "changed text". */
static uint64_t hash_text(const char *data, size_t length) {
    uint64_t hash = 1469598103934665603ULL;
    for(size_t i = 0; i < length; i++) {
        hash ^= (unsigned char)data[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

/**
 * A number that changes whenever the compiler executable changes: its size and
 * modification time. 0 when the executable cannot be found (the cache then
 * relies on PARSE_CACHE_FORMAT alone).
 */
static uint64_t compiler_stamp() {
    static uint64_t stamp = 0;
    static bool known = false;
    if(known) return stamp;
    known = true;

    std::string path;
#ifdef _WIN32
    char buffer[MAX_PATH + 1];
    DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if(length > 0) path.assign(buffer, length);
#else
    path = "/proc/self/exe";    // (Linux; on other systems stat fails and the stamp stays 0)
#endif

    struct stat info;
    if(!path.empty() && stat(path.c_str(), &info) == 0) {
        stamp = ((uint64_t)info.st_size * 1000003ULL) ^ (uint64_t)info.st_mtime;
        if(stamp == 0) stamp = 1;
    }
    return stamp;
}

/** The cache folder ("" = no cache). Created on first use. */
static const std::string &cache_folder() {
    static std::string folder;
    static bool known = false;
    if(known) return folder;
    known = true;

    if(!options.parse_cache)
        return folder;

    if(!options.parse_cache_folder.empty()) {
        folder = options.parse_cache_folder;
    } else {
        const char *home = getenv("HOME");
        if(home == NULL || *home == 0) home = getenv("USERPROFILE");
        if(home == NULL || *home == 0) return folder;    // nowhere to put it

        std::string top = std::string(home) + "/.sharpc";
        File::makeDir(top);
        folder = top + "/cache";
    }

    File::makeDir(folder);

    struct stat info;
    if(stat(folder.c_str(), &info) != 0)
        folder.clear();     // could not be created: compile without a cache
    return folder;
}

static std::string cache_file_for(const std::string &fileName) {
    char name[32];
    snprintf(name, sizeof(name), "%016llx", (unsigned long long)hash_text(fileName.data(), fileName.size()));
    return cache_folder() + "/" + name + ".ast";
}

// ---------------------------------------------------------------------------
// writing
// ---------------------------------------------------------------------------

struct cache_writer {
    std::string out;

    void bytes(const void *data, size_t length) { out.append((const char*)data, length); }
    void u16(uint16_t v) { bytes(&v, sizeof(v)); }
    void u32(uint32_t v) { bytes(&v, sizeof(v)); }
    void i32(int32_t v)  { bytes(&v, sizeof(v)); }
    void u64(uint64_t v) { bytes(&v, sizeof(v)); }
    void text(const std::string &s) { u32((uint32_t)s.size()); bytes(s.data(), s.size()); }

    void node(Ast *ast) {
        u16((uint16_t)ast->getType());
        i32((int32_t)ast->line);
        i32((int32_t)ast->col);

        u32((uint32_t)ast->getTokenCount());
        for(long i = 0; i < ast->getTokenCount(); i++) {
            Token &t = ast->getToken(i);
            u16((uint16_t)t.getId());
            u16((uint16_t)t.getType());
            i32((int32_t)t.getLine());
            i32((int32_t)t.getColumn());
            text(t.getValue());
        }

        u32((uint32_t)ast->getSubAstCount());
        for(long i = 0; i < ast->getSubAstCount(); i++) {
            node(ast->getSubAst(i));
        }
    }
};

void parse_cache_store(const std::string &fileName, const std::string &sourceText, parser *p) {
    if(p == NULL || !p->parsed || cache_folder().empty())
        return;

    // an error or a warning has to be printed again next time: parse again then
    ErrorManager *errors = p->getErrors();
    if(errors != NULL && (errors->hasErrors() || errors->getWarningCount() > 0
                          || errors->getUnfilteredErrorCount() > 0))
        return;

    cache_writer w;
    w.out.reserve(sourceText.size() * 3);
    w.bytes("SHARPAST", 8);
    w.u32(PARSE_CACHE_FORMAT);
    w.u64(compiler_stamp());
    w.u64((uint64_t)sourceText.size());
    w.u64(hash_text(sourceText.data(), sourceText.size()));
    w.text(fileName);
    w.u32((uint32_t)p->size());
    for(size_t i = 0; i < p->size(); i++) {
        w.node(p->astAt((long)i));
    }

    // Written under a temporary name and then renamed, so that a compile
    // running at the same time never reads half a file.
    std::string target = cache_file_for(fileName);
    char unique[32];
#ifdef _WIN32
    snprintf(unique, sizeof(unique), ".%d.tmp", (int)_getpid());
#else
    snprintf(unique, sizeof(unique), ".%d.tmp", (int)getpid());
#endif
    std::string temporary = target + unique;

    FILE *f = fopen(temporary.c_str(), "wb");
    if(f == NULL) return;
    bool written = fwrite(w.out.data(), 1, w.out.size(), f) == w.out.size();
    fclose(f);

    if(written) {
#ifdef _WIN32
        remove(target.c_str());     // rename does not replace a file on Windows
#endif
        if(rename(temporary.c_str(), target.c_str()) != 0)
            remove(temporary.c_str());
    } else {
        remove(temporary.c_str());
    }
}

// ---------------------------------------------------------------------------
// reading
// ---------------------------------------------------------------------------

struct cache_reader {
    const char *at;
    const char *end;
    bool ok;

    cache_reader(const std::string &data) : at(data.data()), end(data.data() + data.size()), ok(true) {}

    bool take(void *into, size_t length) {
        if(!ok || (size_t)(end - at) < length) { ok = false; return false; }
        memcpy(into, at, length);
        at += length;
        return true;
    }
    uint16_t u16() { uint16_t v = 0; take(&v, sizeof(v)); return v; }
    uint32_t u32() { uint32_t v = 0; take(&v, sizeof(v)); return v; }
    int32_t  i32() { int32_t v = 0;  take(&v, sizeof(v)); return v; }
    uint64_t u64() { uint64_t v = 0; take(&v, sizeof(v)); return v; }
    std::string text() {
        uint32_t length = u32();
        if(!ok || (size_t)(end - at) < length) { ok = false; return ""; }
        std::string s(at, length);
        at += length;
        return s;
    }

    /** One node and everything below it. NULL (and ok = false) if the data ends early. */
    Ast *node(int depth) {
        if(depth > 4000) { ok = false; return NULL; }    // not a tree this compiler wrote

        uint16_t type = u16();
        int32_t line = i32();
        int32_t col = i32();
        if(!ok) return NULL;

        Ast *ast = new Ast((ast_type)type, line, col);

        uint32_t tokenCount = u32();
        for(uint32_t i = 0; ok && i < tokenCount; i++) {
            uint16_t id = u16();
            uint16_t tokenType = u16();
            int32_t tokenLine = i32();
            int32_t tokenCol = i32();
            std::string value = text();
            if(ok) ast->addToken(Token(value, (token_id)id, tokenCol, tokenLine, (token_type)tokenType));
        }

        uint32_t childCount = u32();
        for(uint32_t i = 0; ok && i < childCount; i++) {
            Ast *child = node(depth + 1);
            if(child != NULL) ast->addAst(child);
        }

        if(!ok) {
            ast->free();
            delete ast;
            return NULL;
        }
        return ast;
    }
};

bool parse_cache_load(const std::string &fileName, const std::string &sourceText,
                      tokenizer *&outTokenizer, parser *&outParser) {
    if(cache_folder().empty())
        return false;

    std::string data;
    {
        FILE *f = fopen(cache_file_for(fileName).c_str(), "rb");
        if(f == NULL) { cacheMisses++; return false; }

        char chunk[65536];
        size_t got;
        while((got = fread(chunk, 1, sizeof(chunk), f)) > 0) {
            data.append(chunk, got);
        }
        fclose(f);
    }

    cache_reader r(data);
    char magic[8];
    if(!r.take(magic, 8) || memcmp(magic, "SHARPAST", 8) != 0
       || r.u32() != PARSE_CACHE_FORMAT
       || r.u64() != compiler_stamp()
       || r.u64() != (uint64_t)sourceText.size()
       || r.u64() != hash_text(sourceText.data(), sourceText.size())
       || r.text() != fileName
       || !r.ok) {
        cacheMisses++;      // written for other contents or by another compiler: parse, then overwrite
        return false;
    }

    uint32_t count = r.u32();
    std::vector<Ast*> tree;
    tree.reserve(count);
    for(uint32_t i = 0; r.ok && i < count; i++) {
        Ast *ast = r.node(0);
        if(ast != NULL) tree.push_back(ast);
    }

    if(!r.ok || r.at != r.end) {    // damaged file: forget what was read
        for(size_t i = 0; i < tree.size(); i++) {
            tree[i]->free();
            delete tree[i];
        }
        cacheMisses++;
        return false;
    }

    // The tokenizer only splits the text into lines (for error messages and
    // for the copy of the source in the executable); the parser takes the tree.
    outTokenizer = new tokenizer(sourceText, fileName, true, true);
    outParser = new parser(outTokenizer, tree);
    cacheHits++;
    return true;
}
