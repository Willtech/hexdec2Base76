/* b76pipe_stream.c
 *
 * Streaming-friendly reimplementation of b76pipe.c (C11).
 * Produces continuous output while processing input per-byte.
 *
 * Build:
 *   gcc -std=c11 -O2 -Wall -Wextra -o b76pipe_stream b76pipe_stream.c
 *
 * Requires b76pipe.h and base76_alphabet.h (same as original).
 */

#include "b76pipe.h"
#include "base76_alphabet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* BigInt helpers (kept for parity; mark unused to avoid warnings) */
typedef struct {
    unsigned char *data;
    size_t len;
} BigInt;

static BigInt bigint_from_bytes(const unsigned char *buf, size_t len) __attribute__((unused));
static BigInt bigint_from_bytes(const unsigned char *buf, size_t len) {
    BigInt b;
    if (len == 0) {
        b.data = malloc(1);
        if (!b.data) { perror("malloc"); exit(1); }
        b.data[0] = 0;
        b.len = 1;
        return b;
    }
    b.data = malloc(len);
    if (!b.data) { perror("malloc"); exit(1); }
    memcpy(b.data, buf, len);
    b.len = len;
    while (b.len > 1 && b.data[0] == 0) {
        memmove(b.data, b.data + 1, b.len - 1);
        b.len--;
        b.data = realloc(b.data, b.len);
        if (!b.data) { perror("realloc"); exit(1); }
    }
    return b;
}

static BigInt bigint_zero(void) __attribute__((unused));
static BigInt bigint_zero(void) {
    BigInt b;
    b.data = malloc(1);
    if (!b.data) { perror("malloc"); exit(1); }
    b.data[0] = 0;
    b.len = 1;
    return b;
}

static void bigint_free(BigInt *b) __attribute__((unused));
static void bigint_free(BigInt *b) {
    if (!b || !b->data) return;
    free(b->data);
    b->data = NULL;
    b->len = 0;
}

static unsigned int bigint_div_small(BigInt *b, unsigned int divisor) __attribute__((unused));
static unsigned int bigint_div_small(BigInt *b, unsigned int divisor) {
    unsigned int rem = 0;
    for (size_t i = 0; i < b->len; i++) {
        unsigned int cur = (rem << 8) | b->data[i];
        b->data[i] = (unsigned char)(cur / divisor);
        rem = cur % divisor;
    }
    size_t skip = 0;
    while (b->len > 1 && b->data[skip] == 0) {
        skip++;
        b->len--;
    }
    if (skip) {
        memmove(b->data, b->data + skip, b->len);
        b->data = realloc(b->data, b->len);
        if (!b->data) { perror("realloc"); exit(1); }
    }
    return rem;
}

static void bigint_mul_small_add(BigInt *b, unsigned int mul, unsigned int add) __attribute__((unused));
static void bigint_mul_small_add(BigInt *b, unsigned int mul, unsigned int add) {
    unsigned int carry = add;
    for (ssize_t i = (ssize_t)b->len - 1; i >= 0; i--) {
        unsigned int cur = b->data[i] * mul + carry;
        b->data[i] = (unsigned char)(cur & 0xFF);
        carry = cur >> 8;
    }
    while (carry) {
        unsigned char *newdata = malloc(b->len + 1);
        if (!newdata) { perror("malloc"); exit(1); }
        newdata[0] = (unsigned char)(carry & 0xFF);
        memcpy(newdata + 1, b->data, b->len);
        free(b->data);
        b->data = newdata;
        b->len++;
        carry >>= 8;
    }
}

static unsigned char *bigint_to_bytes(const BigInt *b, size_t *out_len) __attribute__((unused));
static unsigned char *bigint_to_bytes(const BigInt *b, size_t *out_len) {
    unsigned char *buf = malloc(b->len);
    if (!buf) { perror("malloc"); exit(1); }
    memcpy(buf, b->data, b->len);
    *out_len = b->len;
    return buf;
}

/* Alphabet lookup */
static int base76_lookup[256];

static void build_lookup(void) {
    if (BASE76_ALPHABET_LEN != 76) {
        fprintf(stderr, "Error: BASE76_ALPHABET must be exactly 76 characters (got %zu)\n",
                (size_t)BASE76_ALPHABET_LEN);
        exit(1);
    }
    for (int i = 0; i < 256; i++) base76_lookup[i] = -1;
    for (int i = 0; i < (int)BASE76_ALPHABET_LEN; i++) {
        unsigned char c = (unsigned char)BASE76_ALPHABET[i];
        if (base76_lookup[c] != -1) {
            fprintf(stderr, "Error: duplicate character '%c' in Base76 alphabet\n", c);
            exit(1);
        }
        base76_lookup[c] = i;
    }
}

static char index_to_base76(unsigned int idx) {
    if (idx >= (unsigned int)BASE76_ALPHABET_LEN) {
        fprintf(stderr, "Error: index out of range: %u\n", idx);
        exit(1);
    }
    return BASE76_ALPHABET[idx];
}

/* decimal (small) -> Base76 (caller frees) */
static char *dec_to_base76(unsigned int n) {
    char tmp[32];
    size_t pos = 0;
    if (n == 0) {
        tmp[pos++] = BASE76_ALPHABET[0];
    } else {
        while (n > 0) {
            unsigned int rem = n % 76;
            n /= 76;
            tmp[pos++] = BASE76_ALPHABET[rem];
        }
    }
    for (size_t i = 0; i < pos/2; i++) {
        char t = tmp[i];
        tmp[i] = tmp[pos-1-i];
        tmp[pos-1-i] = t;
    }
    char *out = malloc(pos + 1);
    if (!out) { perror("malloc"); exit(1); }
    memcpy(out, tmp, pos);
    out[pos] = '\0';
    return out;
}

/* read whole file or stdin (if path == "-") */
static unsigned char *read_file_all(const char *path, size_t *out_len) {
    if (path && strcmp(path, "-") == 0) {
        size_t cap = 4096, len = 0;
        unsigned char *buf = malloc(cap);
        if (!buf) { perror("malloc"); exit(1); }
        for (;;) {
            size_t n = fread(buf + len, 1, cap - len, stdin);
            len += n;
            if (n == 0) break;
            if (len == cap) {
                cap *= 2;
                buf = realloc(buf, cap);
                if (!buf) { perror("realloc"); exit(1); }
            }
        }
        *out_len = len;
        return buf;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error opening '%s': %s\n", path ? path : "(null)", strerror(errno));
        exit(1);
    }
    if (fseek(f, 0, SEEK_END) != 0) { perror("fseek"); exit(1); }
    long s = ftell(f);
    if (s < 0) { perror("ftell"); exit(1); }
    rewind(f);
    unsigned char *buf = malloc((size_t)s + 1);
    if (!buf) { perror("malloc"); exit(1); }
    size_t got = fread(buf, 1, (size_t)s, f);
    if (got != (size_t)s && ferror(f)) { perror("fread"); exit(1); }
    fclose(f);
    *out_len = got;
    return buf;
}

void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [-r] [-i infile] [-o outfile] [-v] [--file-stream] [message]\n", prog);
    fprintf(stderr, "  -r           reverse (decode)\n");
    fprintf(stderr, "  -i infile    read input from infile (use - for stdin)\n");
    fprintf(stderr, "  -o outfile   write final output to outfile (binary safe)\n");
    fprintf(stderr, "  -v           print Stage 1 intermediate tokens (per-byte Base76)\n");
    fprintf(stderr, "  --file-stream  only output Stage 2 (final output) to stdout or -o file\n");
    fprintf(stderr, "  -h, --help   show this help message and exit\n");
    fprintf(stderr, "  --version    print version and exit\n");
    fprintf(stderr, "Version: %s\n", B76PIPE_VERSION);
    exit(1);
}

/* Helper: print single Stage 1 token for a byte to stderr (no newline) */
static void print_stage1_token_from_byte_stderr(unsigned char byte, int first) {
    char *b76 = dec_to_base76((unsigned int)byte);
    if (!b76) { perror("malloc"); exit(1); }
    if (!first) fputc(' ', stderr);
    fputs(b76, stderr);
    free(b76);
    fflush(stderr);
}

/* Helper: write string to output file (no trailing newline) */
static void write_str_out(FILE *outf, const char *s, size_t len) {
    if (fwrite(s, 1, len, outf) != len) { perror("fwrite"); exit(1); }
}

/* Forward per-byte processing helper */
static void process_byte_forward(unsigned char byte, int verbose, int file_stream, int *first_token_ptr, FILE *outf) {
    if (verbose && !file_stream) {
        print_stage1_token_from_byte_stderr(byte, *first_token_ptr);
        *first_token_ptr = 0;
    }

    /* Use dec_to_base76(byte) as Stage1 token and for final output emit bits for each digit */
    char *token = dec_to_base76((unsigned int)byte);
    size_t tlen = strlen(token);
    for (size_t i = 0; i < tlen; i++) {
        unsigned char ch = (unsigned char)token[i];
        int idx = base76_lookup[ch];
        if (idx < 0) {
            fprintf(stderr, "Error: Invalid Base76 character '%c' from dec_to_base76\n", ch);
            exit(1);
        }
        for (int b = 6; b >= 0; b--) {
            unsigned int bit = (idx >> b) & 1u;
            unsigned int val = bit ? 49u : 48u;
            char *b76 = dec_to_base76(val);
            size_t l = strlen(b76);
            write_str_out(outf, b76, l);
            free(b76);
        }
    }
    free(token);
    fflush(outf);
}

/* Reverse helpers: buffer management for reconstructed Base76 digits */
struct rev_state {
    char digit_buf[8];
    size_t digit_buf_len;
    unsigned int bitbuf;
    int bits_collected;
    FILE *outf;
};

static void rev_emit_token_if_possible(struct rev_state *st) {
    while (st->digit_buf_len >= 1) {
        if (st->digit_buf_len == 1) {
            /* wait for second digit to decide two-digit token */
            break;
        }
        unsigned int d0 = (unsigned int)base76_lookup[(unsigned char)st->digit_buf[0]];
        unsigned int d1 = (unsigned int)base76_lookup[(unsigned char)st->digit_buf[1]];
        unsigned int combined = d0 * 76u + d1;
        if (combined <= 255u) {
            unsigned char outb = (unsigned char)combined;
            if (fwrite(&outb, 1, 1, st->outf) != 1) { perror("fwrite"); exit(1); }
            memmove(st->digit_buf, st->digit_buf + 2, st->digit_buf_len - 2);
            st->digit_buf_len -= 2;
        } else {
            unsigned int single = d0;
            unsigned char outb = (unsigned char)single;
            if (fwrite(&outb, 1, 1, st->outf) != 1) { perror("fwrite"); exit(1); }
            memmove(st->digit_buf, st->digit_buf + 1, st->digit_buf_len - 1);
            st->digit_buf_len -= 1;
        }
    }
    fflush(st->outf);
}

static void rev_finalize_tokens(struct rev_state *st) {
    if (st->digit_buf_len == 1) {
        unsigned int d0 = (unsigned int)base76_lookup[(unsigned char)st->digit_buf[0]];
        unsigned char outb = (unsigned char)d0;
        if (fwrite(&outb, 1, 1, st->outf) != 1) { perror("fwrite"); exit(1); }
        st->digit_buf_len = 0;
    } else if (st->digit_buf_len >= 2) {
        while (st->digit_buf_len >= 2) {
            unsigned int d0 = (unsigned int)base76_lookup[(unsigned char)st->digit_buf[0]];
            unsigned int d1 = (unsigned int)base76_lookup[(unsigned char)st->digit_buf[1]];
            unsigned int combined = d0 * 76u + d1;
            unsigned char outb = (unsigned char)combined;
            if (fwrite(&outb, 1, 1, st->outf) != 1) { perror("fwrite"); exit(1); }
            memmove(st->digit_buf, st->digit_buf + 2, st->digit_buf_len - 2);
            st->digit_buf_len -= 2;
        }
    }
    fflush(st->outf);
}

/* Main */
int main(int argc, char **argv) {
    int reverse = 0;
    const char *infile = NULL;
    const char *outfile = NULL;
    int file_stream = 0;
    int verbose = 0;
    int opt;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("%s\n", B76PIPE_VERSION);
            return 0;
        }
        if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
        }
        if (strcmp(argv[i], "--file-stream") == 0) {
            file_stream = 1;
        }
    }

    build_lookup();

    while ((opt = getopt(argc, argv, "ri:o:vh")) != -1) {
        switch (opt) {
        case 'r': reverse = 1; break;
        case 'i': infile = optarg; break;
        case 'o': outfile = optarg; break;
        case 'v': verbose = 1; break;
        case 'h': usage(argv[0]); break;
        default: usage(argv[0]);
        }
    }

    argc -= optind;
    argv += optind;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--file-stream") == 0) file_stream = 1;
    }

    unsigned char *input = NULL;
    size_t input_len = 0;

    if (infile) {
        input = read_file_all(infile, &input_len);
    } else if (argc > 0) {
        size_t total = 0;
        for (int i = 0; i < argc; i++) {
            total += strlen(argv[i]);
            if (i + 1 < argc) total += 1;
        }
        input = malloc(total);
        if (!input) { perror("malloc"); exit(1); }
        size_t pos = 0;
        for (int i = 0; i < argc; i++) {
            size_t l = strlen(argv[i]);
            memcpy(input + pos, argv[i], l);
            pos += l;
            if (i + 1 < argc) input[pos++] = ' ';
        }
        input_len = total;
    } else {
        input = read_file_all("-", &input_len);
    }

    FILE *outf = stdout;
    if (outfile) {
        outf = fopen(outfile, "wb");
        if (!outf) {
            fprintf(stderr, "Error opening output '%s': %s\n", outfile, strerror(errno));
            exit(1);
        }
    }

    if (!reverse) {
        /* Forward streaming: process per-byte from input buffer */
        int first_token = 1;
        for (size_t i = 0; i < input_len; i++) {
            process_byte_forward(input[i], verbose, file_stream, &first_token, outf);
        }
        if (verbose && !file_stream) {
            fputc('\n', stderr);
            fputc('\n', stderr);
            fflush(stderr);
        }
        if (!outfile) fputc('\n', stdout);
    } else {
        /* Reverse streaming: reconstruct digits from bits and emit bytes */
        struct rev_state st;
        st.digit_buf_len = 0;
        st.bitbuf = 0;
        st.bits_collected = 0;
        st.outf = outf;

        for (size_t i = 0; i < input_len; i++) {
            unsigned char ch = input[i];
            int dec = base76_lookup[ch];
            if (dec < 0) {
                fprintf(stderr, "Invalid Base76 character '%c'\n", ch);
                exit(1);
            }
            if (dec == 48 || dec == 49) {
                unsigned int bit = (dec == 49) ? 1u : 0u;
                st.bitbuf = (st.bitbuf << 1) | bit;
                st.bits_collected++;
                if (st.bits_collected == 7) {
                    unsigned int idx = st.bitbuf & 0x7F;
                    if (idx >= BASE76_ALPHABET_LEN) {
                        fprintf(stderr, "Error: 7-bit index out of range: %u\n", idx);
                        exit(1);
                    }
                    char outch = index_to_base76(idx);
                    st.digit_buf[st.digit_buf_len++] = outch;
                    st.bits_collected = 0;
                    st.bitbuf = 0;
                    rev_emit_token_if_possible(&st);
                }
            } else {
                fprintf(stderr, "Invalid encoded bit: %c (DEC=%d)\n", ch, dec);
                exit(1);
            }
        }
        rev_finalize_tokens(&st);
    }

    if (outfile) fclose(outf);
    free(input);
    return 0;
}
