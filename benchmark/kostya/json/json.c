/*
 * JSON - the "json" test of github.com/kostya/benchmarks, written the way
 * json.sharp is: the whole document is parsed into a tree of values, then the
 * x, y and z members of every entry of "coordinates" are averaged. The parser
 * is a plain recursive one, written here (C has none).
 *
 *      json <file>         (the file is made by generate_json.sharp)
 *
 * Measured: parsing and the calculation. The last line is `time: <seconds>`.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum kind { JSON_NULL, JSON_BOOL, JSON_NUMBER, JSON_TEXT, JSON_LIST, JSON_OBJECT };

struct value {
    enum kind kind;
    double number;              /* a number, or 1 / 0 for true / false */
    char *text;                 /* a text */
    struct value **items;       /* a list's values, an object's values */
    char **names;               /* an object's member names, beside `items` */
    int count;
};

static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static const char *skip(const char *at) {
    while (*at == ' ' || *at == '\n' || *at == '\r' || *at == '\t') at++;
    return at;
}

/* the text from the `"` at *at; *at ends up behind its closing `"` */
static char *parse_text(const char **at) {
    const char *from = *at + 1, *to = from;
    while (*to != '"') { if (*to == '\\') to++; to++; }

    char *text = malloc(to - from + 1);
    int n = 0;
    for (const char *c = from; c < to; c++) {
        if (*c == '\\') {
            c++;
            text[n++] = *c == 'n' ? '\n' : *c == 't' ? '\t' : *c == 'r' ? '\r' : *c;
        } else {
            text[n++] = *c;
        }
    }
    text[n] = 0;
    *at = to + 1;
    return text;
}

static struct value *parse_value(const char **at);

static void add(struct value *to, char *name, struct value *item, int *room) {
    if (to->count == *room) {
        *room = *room == 0 ? 4 : *room * 2;
        to->items = realloc(to->items, *room * sizeof(struct value *));
        if (to->kind == JSON_OBJECT) to->names = realloc(to->names, *room * sizeof(char *));
    }
    if (to->kind == JSON_OBJECT) to->names[to->count] = name;
    to->items[to->count++] = item;
}

static struct value *parse_value(const char **at) {
    struct value *made = calloc(1, sizeof(struct value));
    const char *c = skip(*at);
    int room = 0;

    if (*c == '{') {
        made->kind = JSON_OBJECT;
        c = skip(c + 1);
        while (*c != '}') {
            char *name = parse_text(&c);
            c = skip(c) + 1;                    /* the `:` */
            add(made, name, parse_value(&c), &room);
            c = skip(c);
            if (*c == ',') c = skip(c + 1);
        }
        c++;
    } else if (*c == '[') {
        made->kind = JSON_LIST;
        c = skip(c + 1);
        while (*c != ']') {
            add(made, NULL, parse_value(&c), &room);
            c = skip(c);
            if (*c == ',') c = skip(c + 1);
        }
        c++;
    } else if (*c == '"') {
        made->kind = JSON_TEXT;
        made->text = parse_text(&c);
    } else if (*c == 't') {
        made->kind = JSON_BOOL; made->number = 1; c += 4;
    } else if (*c == 'f') {
        made->kind = JSON_BOOL; c += 5;
    } else if (*c == 'n') {
        c += 4;
    } else {
        char *end;
        made->kind = JSON_NUMBER;
        made->number = strtod(c, &end);
        c = end;
    }
    *at = c;
    return made;
}

static struct value *member(const struct value *object, const char *name) {
    for (int i = 0; i < object->count; i++) {
        if (strcmp(object->names[i], name) == 0) return object->items[i];
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2) { printf("usage: json <file>\n"); return 2; }

    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) { printf("cannot read %s\n", argv[1]); return 1; }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *text = malloc(length + 1);
    if (fread(text, 1, length, file) != (size_t) length) return 1;
    text[length] = 0;
    fclose(file);

    double start = now();
    const char *at = text;
    struct value *root = parse_value(&at);
    struct value *coordinates = member(root, "coordinates");
    double x = 0, y = 0, z = 0;
    for (int i = 0; i < coordinates->count; i++) {
        const struct value *entry = coordinates->items[i];
        x += member(entry, "x")->number;
        y += member(entry, "y")->number;
        z += member(entry, "z")->number;
    }
    int count = coordinates->count;
    double seconds = now() - start;

    printf("Coordinate {x: %.17g, y: %.17g, z: %.17g}\n", x / count, y / count, z / count);
    printf("time: %.3f\n", seconds);
    return 0;
}
