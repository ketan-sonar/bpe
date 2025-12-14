#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define min(a, b) ((a < b) ? a : b)

#define swap(Type, a, b) \
    do { \
        Type temp = a; \
        a = b; \
        b = temp; \
    } while (false)

#define da_append(xs, x) \
    do { \
        if ((xs).count >= (xs).capacity) { \
            if ((xs).capacity == 0) (xs).capacity = 256; \
            else (xs).capacity *= 2; \
            (xs).items = realloc((xs).items, (xs).capacity * sizeof(*(xs).items)); \
        } \
        (xs).items[(xs).count++] = (x); \
    } while (false)

typedef uint32_t Token;

typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
} Tokens;

typedef struct {
    Token _0;
    Token _1;
} Byte_Pair;

typedef struct {
    Byte_Pair *items;
    size_t count;
    size_t capacity;
} Byte_Pairs;

typedef struct {
    Byte_Pair key;
    size_t value;
} Byte_Pair_Freq;

typedef struct {
    Byte_Pair_Freq *items;
    size_t count;
    size_t capacity;
} Byte_Pair_Freqs;

int compare_byte_pair_freqs(const void *a, const void * b)
{
    return (int)((Byte_Pair_Freq *)b)->value - (int)((Byte_Pair_Freq *)a)->value;
}

void display_freqs(Byte_Pair_Freq *freqs)
{
    Byte_Pair_Freqs sorted_freqs = {0};
    for (int i = 0; i < hmlen(freqs); i++) {
        da_append(sorted_freqs, freqs[i]);
    }
    qsort(sorted_freqs.items, sorted_freqs.count, sizeof(*sorted_freqs.items),
          compare_byte_pair_freqs);
    for (size_t i = 0; i < min(10, sorted_freqs.count); i++) {
        Byte_Pair_Freq freq = sorted_freqs.items[i];
        printf("(%u, %u) => %zu\n", freq.key._0, freq.key._1, freq.value);
    }
}

void display_tokens(Tokens tokens, Byte_Pairs pairs)
{
    for (size_t i = 0; i < tokens.count; i++) {
        uint32_t token = tokens.items[i];
        assert(token < pairs.count);
        if (pairs.items[token]._0 == token) {
            printf("%c", token);
        } else {
            printf("[%u]", token);
        }
    }
    printf("\n");
}

void dump_pairs(const char *file_path, Byte_Pairs pairs)
{
    FILE *f = fopen(file_path, "wb");
    if (f == NULL) {
        fprintf(stderr, "ERROR: could not open file: %s: %s\n",
                file_path, strerror(errno));
        exit(1);
    }
    fwrite(pairs.items, sizeof(*pairs.items), pairs.count, f);
    fclose(f);
}

void load_pairs(const char *file_path, Byte_Pairs *pairs)
{
    FILE *f = fopen(file_path, "rb");
    if (f == NULL) {
        fprintf(stderr, "ERROR: could not open file: %s: %s\n",
                file_path, strerror(errno));
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);

    size_t item_size = sizeof(*pairs->items);
    assert(n % item_size == 0);

    size_t item_count = n / item_size;
    Byte_Pair *items = malloc(item_count * item_size);
    assert(items);

    size_t read = fread(items, item_size, item_count, f);
    assert(read == item_count);

    for (size_t i = 0; i < item_count; i++) {
        da_append(*pairs, items[i]);
    }
    printf("Loaded %zu tokens\n", pairs->count);

    free(items);
    fclose(f);
}

void generate_pairs(const char *input_file_path, const char *output_file_path)
{
    FILE *f = fopen(input_file_path, "r");
    if (f == NULL) {
        fprintf(stderr, "ERROR: could not read file: %s: %s\n", input_file_path, strerror(errno));
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *text = malloc(n);
    size_t read = fread(text, sizeof(char), n, f);
    assert(read == (size_t)n);
    const int text_size = strlen(text);

    Byte_Pair_Freq *freqs = NULL;
    Byte_Pairs pairs = {0};
    Tokens tokens_in = {0};
    Tokens tokens_out = {0};

    for (int i = 0; i < 256; i++) {
        da_append(pairs, (Byte_Pair){ ._0 = i });
    }

    for (int i = 0; i < text_size; i++) {
        da_append(tokens_in, text[i]);
    }

    while (true) {
        hmfree(freqs);
        freqs = NULL;
        for (size_t i = 0; i < tokens_in.count - 1; i++) {
            Byte_Pair key = { tokens_in.items[i], tokens_in.items[i + 1] };
            ptrdiff_t i = hmgeti(freqs, key);
            if (i < 0) hmput(freqs, key, 1);
            else freqs[i].value += 1;
        }

        ptrdiff_t max_freq_index = 0;
        for (ptrdiff_t i = 1; i < hmlen(freqs); i++) {
            if (freqs[i].value > freqs[max_freq_index].value) {
                max_freq_index = i;
            }
        }
        Byte_Pair_Freq max_freq = freqs[max_freq_index];
        if (max_freq.value <= 1) break;
        da_append(pairs, max_freq.key);

        tokens_out.count = 0;
        for (size_t i = 0; i < tokens_in.count; ) {
            if (i + 1 >= tokens_in.count) {
                da_append(tokens_out, tokens_in.items[i]);
                i++;
            } else {
                Byte_Pair pair = { tokens_in.items[i], tokens_in.items[i + 1] };
                if (memcmp(&pair, &max_freq.key, sizeof(pair)) == 0) {
                    da_append(tokens_out, pairs.count - 1);
                    i += 2;
                } else {
                    da_append(tokens_out, tokens_in.items[i]);
                    i++;
                }
            }
        }
        swap(Tokens, tokens_in, tokens_out);
    }

    dump_pairs(output_file_path, pairs);
    printf("Wrote %zu tokens to %s\n", pairs.count, output_file_path);
}

void usage(const char *program)
{
    fprintf(stderr, "Usage: %s [subcommand]\n", program);
    fprintf(stderr, "  tokenize <input txt file> <output bin file>  Tokenize a .txt file and generate a .bin file.\n");
    fprintf(stderr, "  load <input bin file>                        Load a .bin file and read the tokens in it.\n");
}

int main(int argc, const char **argv)
{
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "ERROR: invalid usage!\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (strncmp(argv[1], "tokenize", 8) == 0) {
        generate_pairs(argv[2], argv[3]);
    } else if (strncmp(argv[1], "load", 4) == 0) {
        Byte_Pairs pairs = {0};
        load_pairs(argv[2], &pairs);
    }

    return EXIT_SUCCESS;
}
