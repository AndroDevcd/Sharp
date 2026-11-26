/*
 * Primes - the "primes" test of github.com/kostya/benchmarks, written the way
 * primes.sharp is: the primes up to the upper bound from the sieve of Atkin,
 * their decimal digits put into a prefix tree, then every prime that starts
 * with the prefix collected by walking the tree breadth first, printed sorted.
 *
 *      primes [upper bound] [prefix]       (default 5000000 and 32338)
 *
 * One difference from the Sharp program: a node's children are an array of
 * ten pointers, one per digit, where Sharp keeps them in a hash map (C has no
 * hash map of its own).
 *
 * Measured: all of it. The last line is `time: <seconds>`.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct node { struct node *children[10]; int terminal; };

static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

/* prime[n] = 1 for every prime n from 5 to limit (2 and 3 are added by the caller) */
static char *sieve(long limit) {
    char *prime = calloc(limit + 1, 1);

    for (long x = 1; x * x < limit; x++) {
        for (long y = 1; y * y < limit; y++) {
            long n = 4 * x * x + y * y;
            if (n <= limit && (n % 12 == 1 || n % 12 == 5)) prime[n] = !prime[n];

            n = 3 * x * x + y * y;
            if (n <= limit && n % 12 == 7) prime[n] = !prime[n];

            n = 3 * x * x - y * y;
            if (x > y && n <= limit && n % 12 == 11) prime[n] = !prime[n];
        }
    }
    for (long r = 5; r * r < limit; r++) {
        if (prime[r]) {
            for (long i = r * r; i < limit; i += r * r) prime[i] = 0;
        }
    }
    return prime;
}

static void add(struct node *root, long prime) {
    char digits[24];
    int length = snprintf(digits, sizeof(digits), "%ld", prime);
    struct node *head = root;

    for (int d = 0; d < length; d++) {
        int key = digits[d] - '0';
        if (head->children[key] == NULL) head->children[key] = calloc(1, sizeof(struct node));
        head = head->children[key];
    }
    head->terminal = 1;
}

static int by_value(const void *a, const void *b) {
    long x = *(const long *) a, y = *(const long *) b;
    return (x > y) - (x < y);
}

/* the primes up to `upper_bound` that start with the digits of `prefix`, ascending; *count: how many (-1: none) */
static long *find(long upper_bound, long prefix, int *count) {
    char *prime = sieve(upper_bound);
    struct node *root = calloc(1, sizeof(struct node));
    add(root, 2);
    add(root, 3);
    for (long p = 5; p <= upper_bound; p++) {
        if (prime[p]) add(root, p);
    }

    char digits[24];
    int length = snprintf(digits, sizeof(digits), "%ld", prefix);
    struct node *head = root;
    for (int d = 0; d < length; d++) {
        head = head->children[digits[d] - '0'];
        if (head == NULL) { *count = -1; return NULL; }
    }

    /* breadth first: a queue of nodes and the number each one spells */
    int room = 64, front = 0, back = 0, found_room = 64, found_count = 0;
    struct node **queue_nodes = malloc(room * sizeof(struct node *));
    long *queue_values = malloc(room * sizeof(long));
    long *found = malloc(found_room * sizeof(long));
    queue_nodes[back] = head;
    queue_values[back++] = prefix;

    while (front < back) {
        struct node *top = queue_nodes[front];
        long value = queue_values[front++];

        if (top->terminal) {
            if (found_count == found_room) { found_room *= 2; found = realloc(found, found_room * sizeof(long)); }
            found[found_count++] = value;
        }
        for (int digit = 0; digit < 10; digit++) {
            if (top->children[digit] == NULL) continue;
            if (back == room) {
                room *= 2;
                queue_nodes = realloc(queue_nodes, room * sizeof(struct node *));
                queue_values = realloc(queue_values, room * sizeof(long));
            }
            queue_nodes[back] = top->children[digit];
            queue_values[back++] = value * 10 + digit;
        }
    }

    qsort(found, found_count, sizeof(long), by_value);
    *count = found_count;
    return found;
}

int main(int argc, char **argv) {
    long upper_bound = argc > 1 ? atol(argv[1]) : 5000000;
    long prefix = argc > 2 ? atol(argv[2]) : 32338;

    double start = now();
    int count;
    long *results = find(upper_bound, prefix, &count);
    double seconds = now() - start;

    if (count < 0) {
        printf("None\n");
    } else {
        printf("[");
        for (int i = 0; i < count; i++) printf(i > 0 ? ", %ld" : "%ld", results[i]);
        printf("]\n");
    }
    printf("time: %.3f\n", seconds);
    return 0;
}
