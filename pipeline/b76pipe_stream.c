/*==============================================================================
 * b76pipe_stream.c — Streaming Base76 encoder/decoder
 *
 * Source Code produced by Willtech 2023–2026
 * Compiled pipeline variant based on hexdec2Base76 project
 * Converted & annotated with reference header by
 * Professor. Damian A. James Williamson Grad. + Copilot
 *
 * Version: B76PIPE_VERSION (see b76pipe.h)
 * License: Willtech Open Technical Artifact License (WOTAL)
 *============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "b76pipe.h"
#include "base76_alphabet.h"

/*==============================================================================
 * Globals
 *============================================================================*/

static unsigned char alphabet[76];   /* Active alphabet */
static unsigned char alpha0;         /* alphabet[0] */
static unsigned char alpha1;         /* alphabet[1] */

/*==============================================================================
 * Usage
 *============================================================================*/

void usage(const char *prog)
{
    dprintf(STDERR_FILENO,
        "Usage: %s [options]\n"
        "\n"
        "Options:\n"
        "  -i <file>        Input file (default: stdin)\n"
        "  -o <file>        Output file (default: stdout)\n"
        "  -v               Verbose: write Stage-1 tokens to stderr\n"
        "  -r               Reverse mode (decode)\n"
        "  --alpha <file>   Load custom Base76 alphabet (first 76 unique chars)\n"
        "  -h, --help       Show this help\n"
        "  --version        Show version\n"
        "\n"
        "Short options may be concatenated: e.g. -rvi\n",
        prog
    );
}

/*==============================================================================
 * Alphabet Loader (first 76 unique 7-bit chars, no sorting)
 *============================================================================*/

static int load_alphabet_file(const char *path)
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
            continue;               /* ignore non-7-bit */

        if (isspace((unsigned char)c))
            continue;               /* ignore whitespace */

        if (seen[c])
            continue;               /* ignore duplicates */

        seen[c] = 1;
        alphabet[count++] = (unsigned char)c;

        if (count == 76)
            break;
    }

    fclose(fp);

    if (count < 76)
        return -2;

    alpha0 = alphabet[0];
    alpha1 = alphabet[1];
    return 0;
}

/*==============================================================================
 * Base76 index lookup
 *============================================================================*/

static int base76_index(unsigned char c)
{
    for (int i = 0; i < 76; i++) {
        if (alphabet[i] == c)
            return i;
    }
    return -1;
}

/*==============================================================================
 * Stage-1: Byte → Base76 token
 *============================================================================*/

static int byte_to_stage1(unsigned char in, unsigned char *buf)
{
    /* 0–75 → 1 char
     * 76–255 → 2 chars: high digit + low digit
     */
    if (in < 76) {
        buf[0] = alphabet[in];
        return 1;
    }

    unsigned int v = in;
    unsigned int hi = v / 76;
    unsigned int lo = v % 76;

    buf[0] = alphabet[hi];
    buf[1] = alphabet[lo];
    return 2;
}

/*==============================================================================
 * Forward: Stage-1 digit → 7-bit → Base76(bitstream)
 *============================================================================*/

static void stage2_emit(int out_fd, int value)
{
    for (int bit = 6; bit >= 0; bit--) {
        unsigned char out = (value & (1 << bit)) ? alpha1 : alpha0;
        write(out_fd, &out, 1);
    }
}

/*==============================================================================
 * Reverse: Base76(bitstream) → bits → 7-bit value
 *============================================================================*/

static int bit_from_char(unsigned char c)
{
    if (c == alpha0) return 0;
    if (c == alpha1) return 1;
    return -1;
}

/*==============================================================================
 * Forward Mode
 *============================================================================*/

static void run_forward(int in_fd, int out_fd, int verbose)
{
    unsigned char in;
    unsigned char tbuf[2];

    while (read(in_fd, &in, 1) == 1) {

        /* Stage-1 */
        int tlen = byte_to_stage1(in, tbuf);

        if (verbose) {
            for (int i = 0; i < tlen; i++)
                write(STDERR_FILENO, &tbuf[i], 1);
        }

        /* Stage-2 */
        for (int i = 0; i < tlen; i++) {
            int idx = base76_index(tbuf[i]);
            if (idx < 0) {
                dprintf(STDERR_FILENO,
                        "Error: Stage-1 digit not in alphabet.\n");
                exit(1);
            }
            stage2_emit(out_fd, idx);
        }
    }
}

/*==============================================================================
 * Reverse Mode
 *============================================================================*/

static void run_reverse(int in_fd, int out_fd)
{
    unsigned char c;
    int bitcount = 0;
    int accum = 0;

    unsigned char stage1_buf[2];
    int stage1_len = 0;

    while (read(in_fd, &c, 1) == 1) {

        int b = bit_from_char(c);
        if (b < 0) {
            dprintf(STDERR_FILENO,
                    "Error: Invalid Base76 bitstream character.\n");
            exit(1);
        }

        accum = (accum << 1) | b;
        bitcount++;

        if (bitcount == 7) {
            /* We have a 7-bit value 0..75 */
            if (accum < 0 || accum >= 76) {
                dprintf(STDERR_FILENO,
                        "Error: 7-bit value out of range.\n");
                exit(1);
            }

            /* Convert to Stage-1 digit */
            stage1_buf[stage1_len++] = alphabet[accum];

            /* If Stage-1 token complete, convert to byte */
            if (stage1_len == 1) {
                /* Could be 1-char or start of 2-char token */
                int idx = base76_index(stage1_buf[0]);
                if (idx < 0) {
                    dprintf(STDERR_FILENO,
                            "Error: Stage-1 digit invalid.\n");
                    exit(1);
                }
                if (idx < 76) {
                    /* Single-char token */
                    unsigned char out = (unsigned char)idx;
                    write(out_fd, &out, 1);
                    stage1_len = 0;
                }
            }
            else if (stage1_len == 2) {
                /* Two-char token */
                int hi = base76_index(stage1_buf[0]);
                int lo = base76_index(stage1_buf[1]);
                if (hi < 0 || lo < 0) {
                    dprintf(STDERR_FILENO,
                            "Error: Stage-1 token invalid.\n");
                    exit(1);
                }
                unsigned char out = (unsigned char)(hi * 76 + lo);
                write(out_fd, &out, 1);
                stage1_len = 0;
            }

            /* Reset for next 7 bits */
            accum = 0;
            bitcount = 0;
        }
    }

    if (bitcount != 0 || stage1_len != 0) {
        dprintf(STDERR_FILENO,
                "Error: Incomplete Base76 bitstream.\n");
        exit(1);
    }
}

/*==============================================================================
 * Main
 *============================================================================*/

int main(int argc, char **argv)
{
    int in_fd = STDIN_FILENO;
    int out_fd = STDOUT_FILENO;

    int verbose = 0;
    int reverse = 0;

    const char *alpha_file = NULL;
    const char *infile = NULL;
    const char *outfile = NULL;

    /* Load built-in alphabet first */
    if (BASE76_ALPHABET_LEN != 76) {
        dprintf(STDERR_FILENO,
                "Error: Built-in alphabet must be exactly 76 chars.\n");
        return 1;
    }
    memcpy(alphabet, BASE76_ALPHABET, 76);
    alpha0 = alphabet[0];
    alpha1 = alphabet[1];

    /* Parse arguments (support concatenated short options) */
    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--version") == 0) {
            dprintf(STDOUT_FILENO, "%s\n", B76PIPE_VERSION);
            return 0;
        }
        if (strcmp(argv[i], "--alpha") == 0) {
            if (++i >= argc) {
                dprintf(STDERR_FILENO, "Error: --alpha requires a file.\n");
                return 1;
            }
            alpha_file = argv[i];
            continue;
        }

        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            /* Short options, possibly concatenated */
            const char *p = argv[i] + 1;
            while (*p) {
                switch (*p) {
                case 'r': reverse = 1; break;
                case 'v': verbose = 1; break;
                case 'i':
                    if (*(p+1) != '\0') {
                        infile = p+1;
                        p += strlen(p+1);
                    } else {
                        if (++i >= argc) {
                            dprintf(STDERR_FILENO,
                                    "Error: -i requires a file.\n");
                            return 1;
                        }
                        infile = argv[i];
                    }
                    goto next_arg;
                case 'o':
                    if (*(p+1) != '\0') {
                        outfile = p+1;
                        p += strlen(p+1);
                    } else {
                        if (++i >= argc) {
                            dprintf(STDERR_FILENO,
                                    "Error: -o requires a file.\n");
                            return 1;
                        }
                        outfile = argv[i];
                    }
                    goto next_arg;
                case 'h':
                    usage(argv[0]);
                    return 0;
                default:
                    dprintf(STDERR_FILENO,
                            "Error: Unknown option -%c\n", *p);
                    return 1;
                }
                p++;
            }
        next_arg:
            continue;
        }

        dprintf(STDERR_FILENO, "Error: Unexpected argument: %s\n", argv[i]);
        return 1;
    }

    /* Load custom alphabet if provided */
    if (alpha_file) {
        int rc = load_alphabet_file(alpha_file);
        if (rc == -1) {
            dprintf(STDERR_FILENO,
                    "Error: Cannot open alphabet file: %s\n", alpha_file);
            return 1;
        }
        if (rc == -2) {
            dprintf(STDERR_FILENO,
                    "Error: Alphabet file must contain at least 76 unique 7-bit characters.\n");
            return 1;
        }
    }

    /* Open input file if -i */
    if (infile) {
        in_fd = open(infile, O_RDONLY);
        if (in_fd < 0) {
            dprintf(STDERR_FILENO,
                    "Error: Cannot open input file: %s\n", infile);
            return 1;
        }
    }

    /* Open output file if -o */
    if (outfile) {
        out_fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0) {
            dprintf(STDERR_FILENO,
                    "Error: Cannot open output file: %s\n", outfile);
            return 1;
        }
    }

    /* Run forward or reverse */
    if (reverse)
        run_reverse(in_fd, out_fd);
    else
        run_forward(in_fd, out_fd, verbose);

    return 0;
}
