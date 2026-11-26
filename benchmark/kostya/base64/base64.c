/*
 * Base64 - the "base64" test of github.com/kostya/benchmarks, written the way
 * base64.sharp is: a string of `a`s is encoded and the result decoded again,
 * `tries` times each, into a newly allocated buffer every time. The encoder
 * and decoder are plain table-driven loops, written here (C has none).
 *
 *      base64 [string size] [tries]        (default 131072 and 8192)
 *
 * Measured: the two loops. The last line is `time: <seconds>`.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int values[256];

static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

/* a new buffer with `data` as base64; *out_size: its length */
static char *encode(const unsigned char *data, size_t size, size_t *out_size) {
    size_t length = (size + 2) / 3 * 4, at = 0, i = 0;
    char *out = malloc(length + 1);

    for (; i + 2 < size; i += 3) {
        unsigned n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out[at++] = letters[n >> 18];
        out[at++] = letters[(n >> 12) & 63];
        out[at++] = letters[(n >> 6) & 63];
        out[at++] = letters[n & 63];
    }
    if (i < size) {
        unsigned n = data[i] << 16;
        if (i + 1 < size) n |= data[i + 1] << 8;
        out[at++] = letters[n >> 18];
        out[at++] = letters[(n >> 12) & 63];
        out[at++] = i + 1 < size ? letters[(n >> 6) & 63] : '=';
        out[at++] = '=';
    }
    out[at] = 0;
    *out_size = at;
    return out;
}

/* a new buffer with what the base64 text `text` stands for; *out_size: its length */
static unsigned char *decode(const char *text, size_t size, size_t *out_size) {
    unsigned char *out = malloc(size / 4 * 3 + 1);
    size_t at = 0;

    for (size_t i = 0; i + 3 < size; i += 4) {
        unsigned n = (values[(unsigned char) text[i]] << 18) | (values[(unsigned char) text[i + 1]] << 12);
        out[at++] = n >> 16;
        if (text[i + 2] != '=') {
            n |= values[(unsigned char) text[i + 2]] << 6;
            out[at++] = (n >> 8) & 255;
            if (text[i + 3] != '=') {
                n |= values[(unsigned char) text[i + 3]];
                out[at++] = n & 255;
            }
        }
    }
    *out_size = at;
    return out;
}

int main(int argc, char **argv) {
    size_t str_size = argc > 1 ? (size_t) atol(argv[1]) : 131072;
    int tries = argc > 2 ? atoi(argv[2]) : 8192;

    for (int i = 0; i < 64; i++) values[(unsigned char) letters[i]] = i;

    unsigned char *str1 = malloc(str_size);
    memset(str1, 'a', str_size);
    size_t size2, size3;
    char *str2 = encode(str1, str_size, &size2);
    unsigned char *str3 = decode(str2, size2, &size3);
    if (size3 != str_size || memcmp(str1, str3, str_size) != 0) { printf("self test failed\n"); return 1; }

    double start = now();
    double s_encoded = 0;
    for (int i = 0; i < tries; i++) {
        size_t size;
        char *made = encode(str1, str_size, &size);
        s_encoded += size;
        free(made);
    }
    double encode_seconds = now() - start;

    double middle = now();
    double s_decoded = 0;
    for (int i = 0; i < tries; i++) {
        size_t size;
        unsigned char *made = decode(str2, size2, &size);
        s_decoded += size;
        free(made);
    }
    double decode_seconds = now() - middle;
    double seconds = now() - start;

    printf("encode aaaa... to %.3s...: %.0f, %.3f\n", str2, s_encoded, encode_seconds);
    printf("decode %.3s... to aaaa...: %.0f, %.3f\n", str2, s_decoded, decode_seconds);
    printf("time: %.3f\n", seconds);
    return 0;
}
