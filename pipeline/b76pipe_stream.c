/*==============================================================================
 * b76pipe_stream.c — Streaming-friendly Base76 7‑bit pipeline encoder/decoder
 *
 * Source Code produced by Willtech 2023–2026
 * Part of the Willtech Base76 Pipeline Suite
 *
 * Design, architecture and reference implementation:
 *   Graduate. Damian Williamson (Willtech)
 *
 * Implementation, documentation and extended diagnostic tooling:
 *   Professor Damian A. James Williamson Grad. + Copilot
 *
 * This program provides a fully reversible Base76 7‑bit pipeline encoder/
 * decoder with strict alphabet validation, deterministic forward/reverse
 * semantics, and a comprehensive internal selftest facility.
 *
 * Stage 1 Input mode:
 *   --stage1
 *       Use Input in Stage 1 form.
 *
 * Selftest modes:
 *   --selftest
 *       Run built‑in diagnostic message and exit.
 *
 *   --selftest "string"
 *       Run selftest using the provided string and exit.
 *
 *   --selftest -i file
 *       Run selftest using the contents of <file> and exit.
 *
 *   --selftest -i file -o file
 *       Run selftest, then forward‑encode <file> to <outfile>,
 *       reverse‑decode <outfile>, compare to original, then exit.
 *
 * License: Willtech Open Technical Artifact License (WOTAL)
 *============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "b76pipe_stream.h"
#include "base76_alphabet.h"

/*==============================================================================
 * BigInt (base‑256, big‑endian)
 *============================================================================*/

typedef struct {
    unsigned char *data;
    size_t len;
} BigInt;

static BigInt
bigint_from_bytes(const unsigned char *buf, size_t len)
{
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

static BigInt
bigint_zero(void)
{
    BigInt b;
    b.data = malloc(1);
    if (!b.data) { perror("malloc"); exit(1); }
    b.data[0] = 0;
    b.len = 1;
    return b;
}

static void
bigint_free(BigInt *b)
{
    if (!b || !b->data) return;
    free(b->data);
    b->data = NULL;
    b->len = 0;
}

static unsigned int
bigint_div_small(BigInt *b, unsigned int divisor)
{
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

static void
bigint_mul_small_add(BigInt *b, unsigned int mul, unsigned int add)
{
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

static unsigned char *
bigint_to_bytes(const BigInt *b, size_t *out_len)
{
    unsigned char *buf = malloc(b->len);
    if (!buf) { perror("malloc"); exit(1); }
    memcpy(buf, b->data, b->len);
    *out_len = b->len;
    return buf;
}

/*==============================================================================
 * Active alphabet + lookup
 *============================================================================*/

static unsigned char alphabet[76];
static int base76_lookup[256];

static void
init_default_alphabet(void)
{
    if (BASE76_ALPHABET_LEN != 76) {
        fprintf(stderr,
                "Error: BASE76_ALPHABET must be exactly 76 characters (got %zu)\n",
                (size_t)BASE76_ALPHABET_LEN);
        exit(1);
    }
    memcpy(alphabet, BASE76_ALPHABET, 76);
}

static void
build_lookup(void)
{
    for (int i = 0; i < 256; i++)
        base76_lookup[i] = -1;

    for (int i = 0; i < 76; i++) {
        unsigned char c = alphabet[i];
        if (base76_lookup[c] != -1) {
            fprintf(stderr,
                    "Error: duplicate character '%c' in Base76 alphabet\n", c);
            exit(1);
        }
        base76_lookup[c] = i;
    }
}

static int
base76_char_to_index(char c)
{
    int idx = base76_lookup[(unsigned char)c];
    if (idx < 0) {
        fprintf(stderr, "Error: Invalid Base76 character '%c'\n", c);
        exit(1);
    }
    return idx;
}

static char
index_to_base76(unsigned int idx)
{
    if (idx >= 76u) {
        fprintf(stderr, "Error: index out of range: %u\n", idx);
        exit(1);
    }
    return alphabet[idx];
}

/*==============================================================================
 * Alphabet loader: --alpha <file>
 *============================================================================*/

static int
load_alphabet_file(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
        return -1;

    int seen[128] = {0};
    int count = 0;

    for (;;) {
        int c = fgetc(fp);
        if (c == EOF)
            break;

        if (c < 0 || c >= 128)
            continue;

        if (isspace((unsigned char)c))
            continue;

        if (seen[c])
            continue;

        seen[c] = 1;
        alphabet[count++] = (unsigned char)c;

        if (count == 76)
            break;
    }

    fclose(fp);

    if (count < 76)
        return -2;

    return 0;
}

/*==============================================================================
 * Base76 conversions (using active alphabet)
 *============================================================================*/

static char *
bytes_to_base76(const unsigned char *buf, size_t len)
{
    BigInt b = bigint_from_bytes(buf, len);

    if (b.len == 1 && b.data[0] == 0) {
        char *out = malloc(2);
        if (!out) { perror("malloc"); exit(1); }
        out[0] = alphabet[0];
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
        digits[n++] = alphabet[rem];
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

static unsigned char *
base76_to_bytes(const char *s, size_t *out_len)
{
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

static char *
dec_to_base76(unsigned int n)
{
    char tmp[32];
    size_t pos = 0;
    if (n == 0) {
        tmp[pos++] = alphabet[0];
    } else {
        while (n > 0) {
            unsigned int rem = n % 76;
            n /= 76;
            tmp[pos++] = alphabet[rem];
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

/*==============================================================================
 * I/O helpers
 *============================================================================*/

static unsigned char *
read_all_fd(int fd, size_t *out_len)
{
    size_t cap = 4096;
    size_t len = 0;
    unsigned char *buf = malloc(cap);
    if (!buf) { perror("malloc"); exit(1); }

    for (;;) {
        unsigned char c;
        ssize_t n = read(fd, &c, 1);
        if (n < 0) {
            perror("read");
            exit(1);
        }
        if (n == 0)
            break;
        if (len == cap) {
            cap *= 2;
            buf = realloc(buf, cap);
            if (!buf) { perror("realloc"); exit(1); }
        }
        buf[len++] = c;
    }

    *out_len = len;
    return buf;
}

/*==============================================================================
 * Usage
 *============================================================================*/

void
usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [options] [message]\n"
        "\n"
        "Options:\n"
        "  -i <file>        Input file (default: stdin)\n"
        "  -o <file>        Output file (default: stdout)\n"
        "  -v               Verbose mode: print Stage‑1 Base76 tokens to stderr\n"
        "  -r               Reverse mode (decode)\n"
        "\n"
        "  --stage1 [s]     Treat input as Stage‑1 tokens instead of raw bytes.\n"
        "                   Forms:\n"
        "                     --stage1 \"18 1S 1T 1d\"\n"
        "                     --stage1 -i <file>\n"
        "                     --stage1 -i -\n"
        "\n"
        "  --alpha <file>   Load custom Base76 alphabet.\n"
        "                   Rules:\n"
        "                     • First 76 unique 7‑bit characters are used\n"
        "                     • Whitespace is ignored\n"
        "                     • Duplicates are ignored\n"
        "                     • Characters >= 128 are ignored\n"
        "                     • Error if fewer than 76 unique chars\n"
        "\n"
        "  --stage1 [s]     Treat input as Stage‑1 Base76 tokens.\n"
        "                   Forms:\n"
        "                     --stage1 \"18 1S 1T 1d\"\n"
        "                     --stage1 -i <file>\n"
        "                     --stage1 -i -\n"
        "\n"
        "  --selftest [s]   Run internal forward/reverse validation.\n"
        "                   Modes:\n"
        "                     --selftest\n"
        "                     --selftest \"string\"\n"
        "                     --selftest -i <file>\n"
        "                     --selftest -i <file> -o <file>\n"
        "\n"
        "  -h, --help       Show this help\n"
        "  --version        Show program version\n",
        prog
    );
    exit(1);
}

/*==============================================================================
 * Stage‑1 verbose printer
 *============================================================================*/

static void
print_stage1_tokens_from_ascii_stderr(const unsigned char *input, size_t len)
{
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

/*==============================================================================
 * Self-test core
 *============================================================================*/

static void
run_selftest(const unsigned char *data, size_t len)
{
    const char *file_i = ".b76pipe_stream.selftest.i";
    const char *file_o = ".b76pipe_stream.selftest.o";

    /* Write original input to .i */
    FILE *fi = fopen(file_i, "wb");
    if (!fi) {
        fprintf(stderr, "Self-test FAILED: cannot open %s for write: %s\n",
                file_i, strerror(errno));
        exit(1);
    }
    if (fwrite(data, 1, len, fi) != len) {
        fprintf(stderr, "Self-test FAILED: write error on %s\n", file_i);
        fclose(fi);
        exit(1);
    }
    fclose(fi);

    /* ---------- Internal forward ---------- */
    char *b76_1 = bytes_to_base76(data, len);
    size_t b76_len = strlen(b76_1);

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

    size_t out_cap = bit_len * 3 + 1;
    char *enc = malloc(out_cap);
    if (!enc) { perror("malloc"); exit(1); }
    size_t enc_len = 0;

    for (size_t i = 0; i < bit_len; i++) {
        unsigned int val = (bits[i] == '0') ? 48u : 49u;
        char *b76 = dec_to_base76(val);
        size_t l = strlen(b76);
        if (enc_len + l + 1 >= out_cap) {
            out_cap = (enc_len + l + 1) * 2;
            enc = realloc(enc, out_cap);
            if (!enc) { perror("realloc"); free(b76_1); free(bits); exit(1); }
        }
        memcpy(enc + enc_len, b76, l);
        enc_len += l;
        free(b76);
    }

    /* Write encoded to .o */
    FILE *fo = fopen(file_o, "wb");
    if (!fo) {
        fprintf(stderr, "Self-test FAILED: cannot open %s for write: %s\n",
                file_o, strerror(errno));
        free(b76_1); free(bits); free(enc);
        exit(1);
    }
    if (fwrite(enc, 1, enc_len, fo) != enc_len) {
        fprintf(stderr, "Self-test FAILED: write error on %s\n", file_o);
        fclose(fo);
        free(b76_1); free(bits); free(enc);
        exit(1);
    }
    fclose(fo);

    /* ---------- Internal reverse from memory ---------- */
    char *ascii01 = malloc(enc_len + 1);
    if (!ascii01) { perror("malloc"); free(b76_1); free(bits); free(enc); exit(1); }
    size_t a_len = 0;

    for (size_t i = 0; i < enc_len; i++) {
        unsigned char ch = (unsigned char)enc[i];
        int dec = base76_char_to_index((char)ch);
        if (dec == 48) ascii01[a_len++] = '0';
        else if (dec == 49) ascii01[a_len++] = '1';
        else {
            fprintf(stderr, "Self-test FAILED: invalid encoded bit in memory.\n");
            free(b76_1); free(bits); free(enc); free(ascii01);
            exit(1);
        }
    }
    ascii01[a_len] = '\0';

    char *b76_back = malloc(a_len / 7 + 2);
    if (!b76_back) { perror("malloc"); free(b76_1); free(bits); free(enc); free(ascii01); exit(1); }
    size_t b76_back_len = 0;
    size_t pos = 0;

    while (a_len - pos >= 7) {
        unsigned int idx = 0;
        for (int bit = 0; bit < 7; bit++) {
            idx <<= 1;
            if (ascii01[pos + bit] == '1')
                idx |= 1;
        }
        pos += 7;
        if (idx >= 76u) {
            fprintf(stderr, "Self-test FAILED: 7-bit index out of range (memory).\n");
            free(b76_1); free(bits); free(enc); free(ascii01); free(b76_back);
            exit(1);
        }
        b76_back[b76_back_len++] = index_to_base76(idx);
    }
    b76_back[b76_back_len] = '\0';

    size_t dec1_len = 0;
    unsigned char *dec1 = base76_to_bytes(b76_back, &dec1_len);

    if (dec1_len != len || memcmp(dec1, data, len) != 0) {
        fprintf(stderr, "Self-test FAILED: forward/reverse mismatch (memory).\n");
        free(b76_1); free(bits); free(enc); free(ascii01); free(b76_back); free(dec1);
        exit(1);
    }
    /* ---------- Reverse from file .o ---------- */
    size_t enc_file_len = 0;
    unsigned char *enc_file = NULL;
    {
        int fd = open(file_o, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "Self-test FAILED: cannot open %s for read: %s\n",
                    file_o, strerror(errno));
            free(b76_1); free(bits); free(enc); free(ascii01); free(b76_back); free(dec1);
            exit(1);
        }
        enc_file = read_all_fd(fd, &enc_file_len);
        close(fd);
    }

    char *ascii01_f = malloc(enc_file_len + 1);
    if (!ascii01_f) { perror("malloc"); free(b76_1); free(bits); free(enc); free(ascii01);
                      free(b76_back); free(dec1); free(enc_file); exit(1); }
    size_t a_f_len = 0;

    for (size_t i = 0; i < enc_file_len; i++) {
        unsigned char ch = (unsigned char)enc_file[i];
        int dec = base76_char_to_index((char)ch);
        if (dec == 48) ascii01_f[a_f_len++] = '0';
        else if (dec == 49) ascii01_f[a_f_len++] = '1';
        else {
            fprintf(stderr, "Self-test FAILED: invalid encoded bit in file.\n");
            free(b76_1); free(bits); free(enc); free(ascii01); free(b76_back); free(dec1);
            free(enc_file); free(ascii01_f);
            exit(1);
        }
    }
    ascii01_f[a_f_len] = '\0';

    char *b76_back_f = malloc(a_f_len / 7 + 2);
    if (!b76_back_f) { perror("malloc"); free(b76_1); free(bits); free(enc); free(ascii01);
                       free(b76_back); free(dec1); free(enc_file); free(ascii01_f); exit(1); }
    size_t b76_back_f_len = 0;
    pos = 0;

    while (a_f_len - pos >= 7) {
        unsigned int idx = 0;
        for (int bit = 0; bit < 7; bit++) {
            idx <<= 1;
            if (ascii01_f[pos + bit] == '1')
                idx |= 1;
        }
        pos += 7;
        if (idx >= 76u) {
            fprintf(stderr, "Self-test FAILED: 7-bit index out of range (file).\n");
            free(b76_1); free(bits); free(enc); free(ascii01); free(b76_back); free(dec1);
            free(enc_file); free(ascii01_f); free(b76_back_f);
            exit(1);
        }
        b76_back_f[b76_back_f_len++] = index_to_base76(idx);
    }
    b76_back_f[b76_back_f_len] = '\0';

    size_t dec2_len = 0;
    unsigned char *dec2 = base76_to_bytes(b76_back_f, &dec2_len);

    if (dec2_len != len || memcmp(dec2, data, len) != 0) {
        fprintf(stderr, "Self-test FAILED: forward/reverse mismatch (file).\n");
        free(b76_1); free(bits); free(enc); free(ascii01); free(b76_back); free(dec1);
        free(enc_file); free(ascii01_f); free(b76_back_f); free(dec2);
        exit(1);
    }

    /* Success */
    printf("b76pipe_stream self-test Okay.\n");

    free(b76_1);
    free(bits);
    free(enc);
    free(ascii01);
    free(b76_back);
    free(dec1);
    free(enc_file);
    free(ascii01_f);
    free(b76_back_f);
    free(dec2);
}

/*==============================================================================
 * Forward pipeline (helper used by main and selftest -i -o check)
 *============================================================================*/

static void
forward_encode_buffer_to_file(const unsigned char *input, size_t input_len,
                              const char *outfile_path)
{
    FILE *outf = fopen(outfile_path, "wb");
    if (!outf) {
        fprintf(stderr, "Error opening output '%s': %s\n",
                outfile_path, strerror(errno));
        exit(1);
    }

    char *b76_1 = bytes_to_base76(input, input_len);
    size_t b76_len = strlen(b76_1);

    size_t bit_cap = b76_len * 7 + 1;
    char *bits = malloc(bit_cap);
    if (!bits) { perror("malloc"); free(b76_1); fclose(outf); exit(1); }
    size_t bit_len = 0;

    for (size_t i = 0; i < b76_len; i++) {
        int idx = base76_char_to_index(b76_1[i]);
        for (int b = 6; b >= 0; b--) {
            bits[bit_len++] = ((idx >> b) & 1) ? '1' : '0';
        }
    }
    bits[bit_len] = '\0';

    size_t out_cap = bit_len * 3 + 1;
    char *outbuf = malloc(out_cap);
    if (!outbuf) { perror("malloc"); free(bits); free(b76_1); fclose(outf); exit(1); }
    size_t out_len = 0;

    for (size_t i = 0; i < bit_len; i++) {
        unsigned int val = (bits[i] == '0') ? 48u : 49u;
        char *b76 = dec_to_base76(val);
        size_t l = strlen(b76);
        if (out_len + l + 1 >= out_cap) {
            out_cap = (out_len + l + 1) * 2;
            outbuf = realloc(outbuf, out_cap);
            if (!outbuf) { perror("realloc"); free(b76); free(bits); free(b76_1);
                           free(outbuf); fclose(outf); exit(1); }
        }
        memcpy(outbuf + out_len, b76, l);
        out_len += l;
        free(b76);
    }

    fwrite(outbuf, 1, out_len, outf);
    fclose(outf);

    free(b76_1);
    free(bits);
    free(outbuf);
}

/*==============================================================================
 * Reverse pipeline (helper used by main and selftest -i -o check)
 *============================================================================*/

static unsigned char *
reverse_decode_file_to_buffer(const char *infile_path, size_t *out_len)
{
    int fd = open(infile_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Error opening '%s': %s\n", infile_path, strerror(errno));
        exit(1);
    }

    size_t input_len = 0;
    unsigned char *input = read_all_fd(fd, &input_len);
    close(fd);

    char *encoded = malloc(input_len + 1);
    if (!encoded) { perror("malloc"); free(input); exit(1); }
    memcpy(encoded, input, input_len);
    encoded[input_len] = '\0';
    free(input);

    size_t elen = strlen(encoded);
    char *ascii01 = malloc(elen + 1);
    if (!ascii01) { perror("malloc"); free(encoded); exit(1); }
    size_t a_len = 0;

    for (size_t i = 0; i < elen; i++) {
        unsigned char ch = (unsigned char)encoded[i];
        if (isspace(ch))
            continue;
        int dec = base76_char_to_index((char)ch);
        if (dec == 48) ascii01[a_len++] = '0';
        else if (dec == 49) ascii01[a_len++] = '1';
        else {
            fprintf(stderr, "Invalid encoded bit: %c (DEC=%d)\n", ch, dec);
            free(encoded);
            free(ascii01);
            exit(1);
        }
    }
    ascii01[a_len] = '\0';

    char *b76_1 = malloc(a_len / 7 + 2);
    if (!b76_1) { perror("malloc"); free(encoded); free(ascii01); exit(1); }
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
        if (idx >= 76u) {
            fprintf(stderr, "Error: 7-bit index out of range: %u\n", idx);
            free(encoded);
            free(ascii01);
            free(b76_1);
            exit(1);
        }
        b76_1[b76_len++] = index_to_base76(idx);
    }
    b76_1[b76_len] = '\0';

    size_t out_bytes_len = 0;
    unsigned char *out_bytes = base76_to_bytes(b76_1, &out_bytes_len);

    free(encoded);
    free(ascii01);
    free(b76_1);

    *out_len = out_bytes_len;
    return out_bytes;
}

/*==============================================================================
 * Stage‑1 parser: Base76 tokens → bytes
 *============================================================================*/

static unsigned char *
stage1_to_bytes(const unsigned char *input, size_t len, size_t *out_len)
{
    unsigned char *out = malloc(len + 1);
    if (!out) { perror("malloc"); exit(1); }

    size_t outpos = 0;
    size_t i = 0;

    while (i < len) {
        while (i < len && isspace(input[i]))
            i++;

        if (i >= len)
            break;

        unsigned int val = 0;
        int digits = 0;

        while (i < len && !isspace(input[i])) {
            unsigned char c = input[i];
            int idx = base76_char_to_index(c);
            val = val * 76 + idx;
            digits++;
            i++;
        }

        if (digits == 0) {
            fprintf(stderr, "Invalid Stage‑1 token: empty token\n");
            free(out);
            exit(1);
        }

        if (val > 255) {
            fprintf(stderr, "Invalid Stage‑1 token: value %u out of byte range\n", val);
            free(out);
            exit(1);
        }

        out[outpos++] = (unsigned char)val;
    }

    *out_len = outpos;
    return out;
}

/*==============================================================================
 * Main
 *============================================================================*/

int
main(int argc, char **argv)
{
    int reverse = 0;
    const char *infile = NULL;
    const char *outfile = NULL;
    const char *alpha_file = NULL;
    int verbose = 0;
    int opt;

    int selftest = 0;
    const char *selftest_str = NULL;

    int stage1 = 0;
    const char *stage1_str = NULL;

    init_default_alphabet();

    /* Manual scan for long options and selftest */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("%s\n", B76PIPE_VERSION);
            return 0;
        }
        if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
        }
        if (strcmp(argv[i], "--alpha") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --alpha requires a file.\n");
                return 1;
            }
            alpha_file = argv[i + 1];
            i++;
            continue;
        }
        if (strcmp(argv[i], "--selftest") == 0) {
            selftest = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                selftest_str = argv[i + 1];
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "--stage1") == 0) {
            stage1 = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                stage1_str = argv[i + 1];
                i++;
            }
            continue;
        }
    }

    if (alpha_file) {
        int rc = load_alphabet_file(alpha_file);
        if (rc == -1) {
            fprintf(stderr,
                    "Error: Cannot open alphabet file: %s\n", alpha_file);
            return 1;
        }
        if (rc == -2) {
            fprintf(stderr,
                    "Error: Alphabet file must contain at least 76 unique 7-bit characters.\n");
            return 1;
        }
    }

    build_lookup();

    int new_argc = 1;
    char **new_argv = malloc(sizeof(char *) * (argc + 1));
    if (!new_argv) { perror("malloc"); exit(1); }
    new_argv[0] = argv[0];

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--alpha") == 0) {
            i++;
            continue;
        }
        if (strcmp(argv[i], "--selftest") == 0) {
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "--stage1") == 0) {
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "--help") == 0 ||
            strcmp(argv[i], "--version") == 0) {
            continue;
        }
        new_argv[new_argc++] = argv[i];
    }

    optind = 1;
    while ((opt = getopt(new_argc, new_argv, "ri:o:vh")) != -1) {
        switch (opt) {
        case 'r': reverse = 1; break;
        case 'i': infile = optarg; break;
        case 'o': outfile = optarg; break;
        case 'v': verbose = 1; break;
        case 'h': usage(new_argv[0]); break;
        default: usage(new_argv[0]);
        }
    }

    int msg_argc = new_argc - optind;
    char **msg_argv = new_argv + optind;

    /* Selftest: string + file is an error */
    if (selftest && selftest_str && infile) {
        fprintf(stderr, "Error: --selftest string and -i file both specified.\n");
        free(new_argv);
        return 1;
    }

/*==========================
 * PURE SELFTEST MODES ONLY
 *==========================*/

if (selftest) {
    unsigned char *st_data = NULL;
    size_t st_len = 0;

    /*
     * Input selection for selftest:
     *
     * 1. --selftest -i file
     *      → read file (or stdin if "-")
     *
     * 2. --selftest "string"
     *      → use provided string
     *
     * 3. --selftest
     *      → use built‑in diagnostic message
     *
     * NOTE:
     *   --stage1 is intentionally ignored in selftest mode.
     *   Selftest always operates on raw bytes.
     */

    if (infile) {
        int fd;
        if (strcmp(infile, "-") == 0)
            fd = STDIN_FILENO;
        else {
            fd = open(infile, O_RDONLY);
            if (fd < 0) {
                fprintf(stderr,
                        "Error opening '%s': %s\n",
                        infile, strerror(errno));
                free(new_argv);
                exit(1);
            }
        }

        st_data = read_all_fd(fd, &st_len);
        if (fd != STDIN_FILENO)
            close(fd);

    } else if (selftest_str) {
        st_len = strlen(selftest_str);
        st_data = malloc(st_len ? st_len : 1);
        if (!st_data) {
            perror("malloc");
            free(new_argv);
            exit(1);
        }
        if (st_len)
            memcpy(st_data, selftest_str, st_len);

    } else {
        const char *def = "This is a b76pipe_stream self-test message.";
        st_len = strlen(def);
        st_data = malloc(st_len);
        if (!st_data) {
            perror("malloc");
            free(new_argv);
            exit(1);
        }
        memcpy(st_data, def, st_len);
    }

    /*
     * Core selftest:
     *   - writes .i and .o
     *   - internal forward/reverse
     *   - file-based reverse
     *   - validates both paths
     */
    run_selftest(st_data, st_len);

    /*
     * Extended mode:
     *   --selftest -i file -o file
     *
     * After the internal selftest succeeds:
     *   - forward encode input to outfile
     *   - reverse decode outfile
     *   - compare to original
     */
    if (infile && outfile) {
        forward_encode_buffer_to_file(st_data, st_len, outfile);

        size_t rev_len = 0;
        unsigned char *rev_buf =
            reverse_decode_file_to_buffer(outfile, &rev_len);

        if (rev_len != st_len ||
            memcmp(rev_buf, st_data, st_len) != 0) {

            fprintf(stderr,
                    "Self-test FAILED: --selftest -i %s -o %s "
                    "forward/reverse mismatch.\n",
                    infile, outfile);

            free(rev_buf);
            free(st_data);
            free(new_argv);
            return 1;
        }

        free(rev_buf);
    }

    free(st_data);
    free(new_argv);
    return 0;
}

    /*==========================
     * NORMAL PIPELINE MODES (with --stage1 support)
     *==========================*/

    unsigned char *input = NULL;
    size_t input_len = 0;

    if (infile) {
        int fd;
        if (strcmp(infile, "-") == 0)
            fd = STDIN_FILENO;
        else {
            fd = open(infile, O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "Error opening '%s': %s\n", infile, strerror(errno));
                free(new_argv);
                exit(1);
            }
        }
        input = read_all_fd(fd, &input_len);
        if (fd != STDIN_FILENO)
            close(fd);
    } else if (stage1 && stage1_str) {
         /* --stage1 "18 1S 1T 1d" */
         input_len = strlen(stage1_str);
         input = malloc(input_len ? input_len : 1);
         if (!input) { perror("malloc"); free(new_argv); exit(1); }
         if (input_len)
             memcpy(input, stage1_str, input_len);
 
     } else if (msg_argc > 0) {
        size_t total = 0;
        for (int i = 0; i < msg_argc; i++) {
            total += strlen(msg_argv[i]);
            if (i + 1 < msg_argc) total += 1;
        }
        input = malloc(total > 0 ? total : 1);
        if (!input) { perror("malloc"); free(new_argv); exit(1); }
        size_t pos = 0;
        for (int i = 0; i < msg_argc; i++) {
            size_t l = strlen(msg_argv[i]);
            memcpy(input + pos, msg_argv[i], l);
            pos += l;
            if (i + 1 < msg_argc) input[pos++] = ' ';
        }
        input_len = pos;
    } else {
        input = read_all_fd(STDIN_FILENO, &input_len);
    }

    FILE *outf = stdout;
    if (outfile) {
        outf = fopen(outfile, "wb");
        if (!outf) {
            fprintf(stderr, "Error opening output '%s': %s\n", outfile, strerror(errno));
            free(new_argv);
            free(input);
            exit(1);
        }
    }

     /* Stage‑1 input mode: convert tokens → bytes before forward pipeline */
     if (stage1 && !reverse) {
         size_t s_len = 0;
         unsigned char *s_bytes = stage1_to_bytes(input, input_len, &s_len);
         free(input);
         input = s_bytes;
         input_len = s_len;
     }

    if (!reverse) {
        if (verbose)
            print_stage1_tokens_from_ascii_stderr(input, input_len);

         /* Reverse mode unchanged by --stage1:
          *   --stage1 is accepted but ignored in reverse mode.
          *   Output is raw ASCII bytes as usual.
          */

        char *b76_1 = bytes_to_base76(input, input_len);
        size_t b76_len = strlen(b76_1);

        size_t bit_cap = b76_len * 7 + 1;
        char *bits = malloc(bit_cap);
        if (!bits) { perror("malloc"); free(new_argv); free(input); free(b76_1); exit(1); }
        size_t bit_len = 0;

        for (size_t i = 0; i < b76_len; i++) {
            int idx = base76_char_to_index(b76_1[i]);
            for (int b = 6; b >= 0; b--) {
                bits[bit_len++] = ((idx >> b) & 1) ? '1' : '0';
            }
        }
        bits[bit_len] = '\0';

        size_t out_cap = bit_len * 3 + 1;
        char *outbuf = malloc(out_cap);
        if (!outbuf) { perror("malloc"); free(bits); free(b76_1); free(new_argv); free(input); exit(1); }
        size_t out_len = 0;

        for (size_t i = 0; i < bit_len; i++) {
            unsigned int val = (bits[i] == '0') ? 48u : 49u;
            char *b76 = dec_to_base76(val);
            size_t l = strlen(b76);
            if (out_len + l + 1 >= out_cap) {
                out_cap = (out_len + l + 1) * 2;
                outbuf = realloc(outbuf, out_cap);
                if (!outbuf) { perror("realloc"); free(b76); free(bits); free(b76_1); free(outbuf); free(new_argv); free(input); exit(1); }
            }
            memcpy(outbuf + out_len, b76, l);
            out_len += l;
            free(b76);
        }

        fwrite(outbuf, 1, out_len, outf);
        if (!outfile)
            fputc('\n', outf);

        free(b76_1);
        free(bits);
        free(outbuf);
    } else {

        /*==============================================================
         * Reverse mode with --stage1:
         *   Decode Stage‑1 tokens directly back to bytes.
         *   This bypasses Stage‑2 decoding entirely.
         *==============================================================*/
        if (stage1) {
            size_t s_len = 0;
            unsigned char *s_bytes = stage1_to_bytes(input, input_len, &s_len);

            if (verbose)
                print_stage1_tokens_from_ascii_stderr(s_bytes, s_len);

            fwrite(s_bytes, 1, s_len, outf);

            free(s_bytes);
            free(new_argv);
            free(input);
            if (outfile)
                fclose(outf);
            return 0;
        }

        /*==============================================================
         * Normal Stage‑2 reverse pipeline
         *==============================================================*/

        char *encoded = malloc(input_len + 1);
        if (!encoded) { perror("malloc"); free(new_argv); free(input); exit(1); }
        memcpy(encoded, input, input_len);
        encoded[input_len] = '\0';

        size_t elen = strlen(encoded);
        char *ascii01 = malloc(elen + 1);
        if (!ascii01) { perror("malloc"); free(encoded); free(new_argv); free(input); exit(1); }
        size_t a_len = 0;

        for (size_t i = 0; i < elen; i++) {
            unsigned char ch = (unsigned char)encoded[i];
            if (isspace(ch))
                continue;
            int dec = base76_char_to_index((char)ch);
            if (dec == 48) ascii01[a_len++] = '0';
            else if (dec == 49) ascii01[a_len++] = '1';
            else {
                fprintf(stderr, "Invalid encoded bit: %c (DEC=%d)\n", ch, dec);
                free(encoded);
                free(ascii01);
                free(new_argv);
                free(input);
                exit(1);
            }
        }
        ascii01[a_len] = '\0';

        char *b76_1 = malloc(a_len / 7 + 2);
        if (!b76_1) { perror("malloc"); free(encoded); free(ascii01); free(new_argv); free(input); exit(1); }
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
            if (idx >= 76u) {
                fprintf(stderr, "Error: 7-bit index out of range: %u\n", idx);
                free(encoded);
                free(ascii01);
                free(b76_1);
                free(new_argv);
                free(input);
                exit(1);
            }
            b76_1[b76_len++] = index_to_base76(idx);
        }
        b76_1[b76_len] = '\0';

        size_t out_len = 0;
        unsigned char *out_bytes = base76_to_bytes(b76_1, &out_len);

        if (verbose)
            print_stage1_tokens_from_ascii_stderr(out_bytes, out_len);

        fwrite(out_bytes, 1, out_len, outf);

        free(encoded);
        free(ascii01);
        free(b76_1);
        free(out_bytes);
    }

    if (outfile)
        fclose(outf);
    free(input);
    free(new_argv);
    return 0;
}

