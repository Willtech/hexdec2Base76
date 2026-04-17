/* b76pipe_stream.c
 *
 * Streaming-friendly reimplementation of b76pipe.c
 * Preserves CLI, flags, and exact IO semantics.
 *
 * Build: gcc -std=c11 -O2 -o b76pipe_stream b76pipe_stream.c
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

/* Reuse BigInt helpers from original but keep them local here for clarity */
typedef struct {
    unsigned char *data;
    size_t len;
} BigInt;

/* --- BigInt helpers (same semantics as original) --- */

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

static BigInt bigint_zero(void) {
    BigInt b;
    b.data = malloc(1);
    if (!b.data) { perror("malloc"); exit(1); }
    b.data[0] = 0;
    b.len = 1;
    return b;
}

static void bigint_free(BigInt *b) {
    if (!b || !b->data) return;
    free(b->data);
    b->data = NULL;
    b->len = 0;
}

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

static unsigned char *bigint_to_bytes(const BigInt *b, size_t *out_len) {
    unsigned char *buf = malloc(b->len);
    if (!buf) { perror("malloc"); exit(1); }
    memcpy(buf, b->data, b->len);
    *out_len = b->len;
    return buf;
}

/* --- Alphabet lookup --- */

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

/* bytes -> Base76 (caller frees) */
static char *bytes_to_base76(const unsigned char *buf, size_t len) {
    BigInt b = bigint_from_bytes(buf, len);
    if (b.len == 1 && b.data[0] == 0) {
        char *out = malloc(2);
        if (!out) { perror("malloc"); exit(1); }
        out[0] = BASE76_ALPHABET[0];
        out[1] = '\0';
        bigint_free(&b);
        return out;
    }
    size_t cap = 32;
    char *digits = malloc(cap);
    if (!digits) { perror("malloc"); exit(1); }
    size_t n = 0;
    while (!(b.len == 1 && b.data[0] == 0)) {
        unsigned int rem = bigint_div_small(&b, 76);
        if (n + 1 >= cap) {
            cap *= 2;
            digits = realloc(digits, cap);
            if (!digits) { perror("realloc"); exit(1); }
        }
        digits[n++] = BASE76_ALPHABET[rem];
    }
    for (size_t i = 0; i < n/2; i++) {
        char t = digits[i];
        digits[i] = digits[n-1-i];
        digits[n-1-i] = t;
    }
    char *out = malloc(n + 1);
    if (!out) { perror("malloc"); exit(1); }
    memcpy(out, digits, n);
    out[n] = '\0';
    free(digits);
    bigint_free(&b);
    return out;
}

/* Base76 -> bytes (caller frees) */
static unsigned char *base76_to_bytes(const char *s, size_t *out_len) {
    BigInt b = bigint_zero();
    size_t slen = strlen(s);
    for (size_t i = 0; i < slen; i++) {
        unsigned char ch = (unsigned char)s[i];
        int idx = base76_lookup[ch];
        if (idx < 0) {
            fprintf(stderr, "Error: Invalid Base76 character '%c'\n", ch);
            exit(1);
        }
        bigint_mul_small_add(&b, 76, (unsigned int)idx);
    }
    unsigned char *bytes = bigint_to_bytes(&b, out_len);
    bigint_free(&b);
    return bytes;
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

static int base76_char_to_index(char c) {
    int idx = base76_lookup[(unsigned char)c];
    if (idx < 0) {
        fprintf(stderr, "Error: Invalid Base76 character '%c'\n", c);
        exit(1);
    }
    return idx;
}

static char index_to_base76(unsigned int idx) {
    if (idx >= (unsigned int)BASE76_ALPHABET_LEN) {
        fprintf(stderr, "Error: index out of range: %u\n", idx);
        exit(1);
    }
    return BASE76_ALPHABET[idx];
}

/* --- Streaming helpers --- */

/* Read entire file into memory for file paths (keeps parity with original for files).
 * For stdin ("-") we stream into a dynamically growing buffer as original did.
 */
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

/* Print Stage 1 tokens for an ASCII byte sequence to stderr (same as original) */
static void print_stage1_tokens_from_ascii_stderr(const unsigned char *input, size_t len) {
    for (size_t i = 0; i < len; i++) {
        unsigned int byte = input[i];
        char *b76 = dec_to_base76(byte);
        if (!b76) { perror("malloc"); exit(1); }
        if (i > 0) fputc(' ', stderr);
        fputs(b76, stderr);
        free(b76);
    }
    fputc('\n', stderr);
    fputc('\n', stderr);
    fflush(stderr);
}

/* --- Main --- */

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
        if (verbose && !file_stream) {
            print_stage1_tokens_from_ascii_stderr(input, input_len);
        }

        /* Forward: convert entire input bytes -> Base76 string (same as original) */
        char *b76_1 = bytes_to_base76(input, input_len);
        size_t b76_len = strlen(b76_1);

        /* Build bit string (7 bits per Base76 index) */
        size_t bit_cap = b76_len * 7 + 1;
        char *bits = malloc(bit_cap);
        if (!bits) { perror("malloc"); exit(1); }
        size_t bit_len = 0;

        for (size_t i = 0; i < b76_len; i++) {
            int idx = base76_char_to_index(b76_1[i]);
            for (int b = 6; b >= 0; b--) {
                bits[bit_len++] = ((idx >> b) & 1) ? '1' : '0';
            }
        }
        bits[bit_len] = '\0';

        /* Convert each '0'/'1' to decimal 48/49 then to Base76 and append */
        size_t out_cap = bit_len * 3 + 1;
        char *outbuf = malloc(out_cap);
        if (!outbuf) { perror("malloc"); exit(1); }
        size_t out_len = 0;

        for (size_t i = 0; i < bit_len; i++) {
            unsigned int val = (bits[i] == '0') ? 48u : 49u;
            char *b76 = dec_to_base76(val);
            size_t l = strlen(b76);
            if (out_len + l + 1 >= out_cap) {
                out_cap = (out_len + l + 1) * 2;
                outbuf = realloc(outbuf, out_cap);
                if (!outbuf) { perror("realloc"); exit(1); }
            }
            memcpy(outbuf + out_len, b76, l);
            out_len += l;
            free(b76);
        }

        if (outfile) {
            fwrite(outbuf, 1, out_len, outf);
        } else {
            fwrite(outbuf, 1, out_len, stdout);
            fputc('\n', stdout);
        }

        free(b76_1);
        free(bits);
        free(outbuf);
    } else {
        /* Reverse pipeline */
        char *encoded = malloc(input_len + 1);
        if (!encoded) { perror("malloc"); exit(1); }
        memcpy(encoded, input, input_len);
        encoded[input_len] = '\0';

        size_t elen = strlen(encoded);
        char *ascii01 = malloc(elen + 1);
        if (!ascii01) { perror("malloc"); exit(1); }
        size_t a_len = 0;

        for (size_t i = 0; i < elen; i++) {
            int dec = base76_char_to_index(encoded[i]);
            if (dec == 48) ascii01[a_len++] = '0';
            else if (dec == 49) ascii01[a_len++] = '1';
            else {
                fprintf(stderr, "Invalid encoded bit: %c (DEC=%d)\n", encoded[i], dec);
                exit(1);
            }
        }
        ascii01[a_len] = '\0';

        char *b76_1 = malloc(a_len / 7 + 2);
        if (!b76_1) { perror("malloc"); exit(1); }
        size_t b76_len = 0;
        size_t pos = 0;

        while (a_len - pos >= 7) {
            unsigned int idx = 0;
            for (int bit = 0; bit < 7; bit++) {
                idx <<= 1;
                if (ascii01[pos + bit] == '1')
                    idx |= 1;
            }
            pos += 7;
            if (idx >= BASE76_ALPHABET_LEN) {
                fprintf(stderr, "Error: 7-bit index out of range: %u\n", idx);
                exit(1);
            }
            b76_1[b76_len++] = index_to_base76(idx);
        }
        b76_1[b76_len] = '\0';

        size_t out_len = 0;
        unsigned char *out_bytes = base76_to_bytes(b76_1, &out_len);

        if (verbose && !file_stream) {
            print_stage1_tokens_from_ascii_stderr(out_bytes, out_len);
        }

        if (outfile) {
            fwrite(out_bytes, 1, out_len, outf);
        } else {
            fwrite(out_bytes, 1, out_len, stdout);
        }

        free(encoded);
        free(ascii01);
        free(b76_1);
        free(out_bytes);
    }

    if (outfile) fclose(outf);
    free(input);
    return 0;
}
