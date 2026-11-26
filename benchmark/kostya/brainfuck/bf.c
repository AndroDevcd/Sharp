/*
 * Brainfuck interpreter - the "brainfuck" test of github.com/kostya/benchmarks,
 * written the way bf.sharp is: the program text is parsed once into a tree of
 * operations (a loop holds its body), then run on a tape that doubles when the
 * pointer runs off its end. Output goes into a checksum, printed at the end.
 *
 *      bf <program.b>
 *
 * Measured: parsing and running the program. The last line is `time: <seconds>`.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* one operation: 1 = add `val` to the cell, 2 = move by `val`, 3 = loop over `body`, 4 = print the cell */
struct op { int kind; int val; struct op *body; int count; };

struct tape { int *cells; int size; int pos; };
struct printer { int sum1, sum2; };

static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

/* the operations from *at on, up to the `]` that closes them (or the end); *count: how many */
static struct op *parse(const char *text, int length, int *at, int *count) {
    int room = 8, n = 0;
    struct op *ops = malloc(room * sizeof(struct op));

    while (*at < length) {
        char c = text[(*at)++];
        struct op made = { 0, 0, NULL, 0 };

        if (c == '+') { made.kind = 1; made.val = 1; }
        else if (c == '-') { made.kind = 1; made.val = -1; }
        else if (c == '>') { made.kind = 2; made.val = 1; }
        else if (c == '<') { made.kind = 2; made.val = -1; }
        else if (c == '.') { made.kind = 4; }
        else if (c == '[') { made.kind = 3; made.body = parse(text, length, at, &made.count); }
        else if (c == ']') break;
        else continue;

        if (n == room) { room *= 2; ops = realloc(ops, room * sizeof(struct op)); }
        ops[n++] = made;
    }
    *count = n;
    return ops;
}

static void move(struct tape *t, int by) {
    t->pos += by;
    while (t->pos >= t->size) {
        t->cells = realloc(t->cells, t->size * 2 * sizeof(int));
        memset(t->cells + t->size, 0, t->size * sizeof(int));
        t->size *= 2;
    }
}

static void put(struct printer *p, int n) {
    p->sum1 = (p->sum1 + n) % 255;
    p->sum2 = (p->sum2 + p->sum1) % 255;
}

static void run(const struct op *program, int count, struct tape *t, struct printer *p) {
    for (int i = 0; i < count; i++) {
        const struct op *o = &program[i];
        switch (o->kind) {
            case 1: t->cells[t->pos] += o->val; break;
            case 2: move(t, o->val); break;
            case 3: while (t->cells[t->pos] > 0) run(o->body, o->count, t, p); break;
            default: put(p, t->cells[t->pos]);
        }
    }
}

static int run_text(const char *text, int length) {
    int at = 0, count;
    struct op *program = parse(text, length, &at, &count);
    struct tape t = { calloc(1, sizeof(int)), 1, 0 };
    struct printer p = { 0, 0 };
    run(program, count, &t, &p);
    return (p.sum2 << 8) | p.sum1;
}

int main(int argc, char **argv) {
    if (argc < 2) { printf("usage: bf <program.b>\n"); return 2; }

    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) { printf("cannot read %s\n", argv[1]); return 1; }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *text = malloc(length + 1);
    if (fread(text, 1, length, file) != (size_t) length) return 1;
    fclose(file);

    double start = now();
    int checksum = run_text(text, (int) length);
    double seconds = now() - start;

    printf("Output checksum: %d\n", checksum);
    printf("time: %.3f\n", seconds);
    return 0;
}
