/*
===============================================================================
 b76pipe.c
 Single-binary Base76 pipeline with optional custom alphabet.

 Features:
   - Forward and reverse pipeline (-r)
   - Input from file (-i) or message argument
   - Output to file (-o) or stdout
   - Custom alphabet via --alpha <file>
   - 7-bit safe alphabet (ASCII < 128)
   - Help (-h, --help) and version (--version)
   - Streaming operation (no full-file buffering)

 Version: 1.0.0.2026.04.15

 Source Code produced by Willtech 2023–2026
 Compiled pipeline variant based on hexdec2Base76 project
 Converted & annotated with reference header by
 Professor. Damian A. James Williamson Grad. + Copilot
===============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "base76_alphabet.h"

#define TOOL_NAME    "b76pipe"
#define TOOL_VERSION "1.0.0.2026.04.15"

#define BASE76_LEN 76

/* ---------------------------------------------------------------------------
   Global alphabet (default from header, override via --alpha)
--------------------------------------------------------------------------- */

static char active_alphabet[BASE76_LEN + 1] = BASE76_ALPHABET;

/* ---------------------------------------------------------------------------
   Alphabet loader for --alpha

   Rules:
     - Read file
     - Ignore whitespace
     - Ignore chars >= 128 (non 7-bit)
     - Keep only first 76 UNIQUE chars
     - If fewer than 76 unique chars -> error
--------------------------------------------------------------------------- */

static int load_alphabet_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "%s: cannot open alphabet file '%s'\n", TOOL_NAME, path);
        return -1;
    }

    int seen[128] = {0};
    int count = 0;
    int c;

    while ((c = fgetc(f)) != EOF) {
        if (c < 0 || c >= 128) {
            continue; /* ignore non 7-bit */
        }
        if (isspace((unsigned char)c)) {
            continue; /* ignore whitespace */
        }
        if (seen[c]) {
            continue; /* ignore duplicates */
        }
        seen[c] = 1;
        active_alphabet[count++] = (char)c;
        if (count == BASE76_LEN) {
            break;
        }
    }

    fclose(f);

    if (count < BASE76_LEN) {
        fprintf(stderr,
                "%s: alphabet file '%s' must contain at least %d unique 7-bit characters\n",
                TOOL_NAME, path, BASE76_LEN);
        return -2;
    }

    active_alphabet[BASE76_LEN] = '\0';
    return 0;
}

/* ---------------------------------------------------------------------------
   Base76 helpers (using active_alphabet)
--------------------------------------------------------------------------- */

static int char_to_val(char c) {
    const char *p = strchr(active_alphabet, c);
    return p ? (int)(p - active_alphabet) : -1;
}

static void dec_to_base76(unsigned long long n, char *out, size_t out_sz) {
    char buf[32];
    int i = 0;

    if (n == 0) {
        if (out_sz < 2) {
            out[0] = '\0';
            return;
        }
        out[0] = active_alphabet[0];
        out[1] = '\0';
        return;
    }

    while (n > 0 && i < (int)(sizeof(buf) - 1)) {
        buf[i++] = active_alphabet[n % BASE76_LEN];
        n /= BASE76_LEN;
    }
    buf[i] = '\0';

    if ((size_t)(i + 1) > out_sz) {
        out[0] = '\0';
        return;
    }

    for (int j = 0; j < i; j++) {
        out[j] = buf[i - 1 - j];
    }
    out[i] = '\0';
}

static unsigned long long base76_to_dec(const char *s) {
    unsigned long long val = 0;
    while (*s) {
        int d = char_to_val(*s++);
        if (d < 0) {
            fprintf(stderr, "%s: invalid Base76 character '%c'\n", TOOL_NAME, s[-1]);
            exit(1);
        }
        val = val * BASE76_LEN + d;
    }
    return val;
}

/* ---------------------------------------------------------------------------
   Streaming pipeline:
     Forward:
       - read bytes
       - for each byte: encode as decimal -> Base76
       - separate tokens with space, final newline
     Reverse:
       - read Base76 tokens separated by whitespace
       - decode each to decimal -> byte
--------------------------------------------------------------------------- */

static int pipeline_forward(FILE *in, FILE *out) {
    int c;
    int first = 1;

    while ((c = fgetc(in)) != EOF) {
        unsigned char b = (unsigned char)c;
        char enc[32];

        dec_to_base76((unsigned long long)b, enc, sizeof(enc));

        if (!first) {
            fputc(' ', out);
        }
        first = 0;

        fputs(enc, out);
    }

    fputc('\n', out);
    return 0;
}

static int pipeline_reverse(FILE *in, FILE *out) {
    char token[64];
    int ti = 0;
    int c;

    while ((c = fgetc(in)) != EOF) {
        if (isspace((unsigned char)c)) {
            if (ti > 0) {
                token[ti] = '\0';
                unsigned long long v = base76_to_dec(token);
                if (v > 255) {
                    fprintf(stderr, "%s: decoded value %llu out of byte range\n",
                            TOOL_NAME, v);
                    return 1;
                }
                fputc((unsigned char)v, out);
                ti = 0;
            }
            continue;
        }

        if (ti < (int)sizeof(token) - 1) {
            token[ti++] = (char)c;
        } else {
            fprintf(stderr, "%s: token too long in input\n", TOOL_NAME);
            return 1;
        }
    }

    if (ti > 0) {
        token[ti] = '\0';
        unsigned long long v = base76_to_dec(token);
        if (v > 255) {
            fprintf(stderr, "%s: decoded value %llu out of byte range\n",
                    TOOL_NAME, v);
            return 1;
        }
        fputc((unsigned char)v, out);
    }

    return 0;
}

/* ---------------------------------------------------------------------------
   CLI: parsing and wrapper
--------------------------------------------------------------------------- */

static void print_help(void) {
    printf("%s - Base76 pipeline (forward and reverse)\n", TOOL_NAME);
    printf("\nUsage:\n");
    printf("  %s [-r] [-i infile] [-o outfile] [--alpha alphabet_file] [message]\n",
           TOOL_NAME);
    printf("\nOptions:\n");
    printf("  -r                 Reverse pipeline (Base76 -> bytes)\n");
    printf("  -i infile          Read input from file (ignores message argument)\n");
    printf("  -o outfile         Write output to file (default: stdout)\n");
    printf("  --alpha file       Load custom Base76 alphabet from file\n");
    printf("  -h, --help         Show this help message\n");
    printf("  --version          Show version information\n");
    printf("\nAlphabet file rules (--alpha):\n");
    printf("  - Only 7-bit characters (ASCII < 128) are considered\n");
    printf("  - Whitespace is ignored\n");
    printf("  - Duplicate characters are ignored\n");
    printf("  - The first 76 unique 7-bit characters form the alphabet\n");
    printf("  - Fewer than 76 unique characters -> error\n");
    printf("\nInput size:\n");
    printf("  b76pipe operates as a streaming pipeline and does not load the entire\n");
    printf("  input into memory. Forward mode processes input byte-by-byte, and\n");
    printf("  reverse mode processes Base76 tokens one at a time. This allows\n");
    printf("  processing of arbitrarily large inputs, limited only by I/O capacity.\n");
    printf("\nOutput behaviour:\n");
    printf("  Output is produced incrementally as input is processed. Large inputs\n");
    printf("  may generate large output streams. When writing to a terminal, consider\n");
    printf("  using -o outfile to avoid overwhelming the display.\n");
}

static void print_version(void) {
    printf("%s version %s\n", TOOL_NAME, TOOL_VERSION);
}

static int b76pipe_main(int reverse,
                        const char *infile,
                        const char *outfile,
                        const char *message) {
    FILE *in = NULL;
    FILE *out = NULL;
    int rc = 0;

    if (infile) {
        in = fopen(infile, "rb");
        if (!in) {
            fprintf(stderr, "%s: cannot open input file '%s'\n", TOOL_NAME, infile);
            return 1;
        }
    } else {
        if (message) {
            /* Use a temporary file as a simple portable in-memory stream */
            FILE *tmp = tmpfile();
            if (!tmp) {
                fprintf(stderr, "%s: cannot create temporary stream\n", TOOL_NAME);
                return 1;
            }
            fputs(message, tmp);
            rewind(tmp);
            in = tmp;
        } else {
            fprintf(stderr, "%s: no input provided (use -i or message)\n", TOOL_NAME);
            return 1;
        }
    }

    if (outfile) {
        out = fopen(outfile, "wb");
        if (!out) {
            fprintf(stderr, "%s: cannot open output file '%s'\n", TOOL_NAME, outfile);
            if (in) fclose(in);
            return 1;
        }
    } else {
        out = stdout;
    }

    if (reverse) {
        rc = pipeline_reverse(in, out);
    } else {
        rc = pipeline_forward(in, out);
    }

    if (in) fclose(in);
    if (outfile && out) fclose(out);

    return rc;
}

int main(int argc, char *argv[]) {
    int reverse = 0;
    const char *infile = NULL;
    const char *outfile = NULL;
    const char *alpha_file = NULL;
    const char *message = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            print_help();
            return 0;
        }
        if (!strcmp(argv[i], "--version")) {
            print_version();
            return 0;
        }
        if (!strcmp(argv[i], "-r")) {
            reverse = 1;
            continue;
        }
        if (!strcmp(argv[i], "-i")) {
            if (i + 1 < argc) {
                infile = argv[++i];
            } else {
                fprintf(stderr, "%s: -i requires a file argument\n", TOOL_NAME);
                return 1;
            }
            continue;
        }
        if (!strcmp(argv[i], "-o")) {
            if (i + 1 < argc) {
                outfile = argv[++i];
            } else {
                fprintf(stderr, "%s: -o requires a file argument\n", TOOL_NAME);
                return 1;
            }
            continue;
        }
        if (!strcmp(argv[i], "--alpha")) {
            if (i + 1 < argc) {
                alpha_file = argv[++i];
            } else {
                fprintf(stderr, "%s: --alpha requires a file argument\n", TOOL_NAME);
                return 1;
            }
            continue;
        }
        if (!message) {
            message = argv[i];
        }
    }

    if (alpha_file) {
        if (load_alphabet_file(alpha_file) != 0) {
            return 1;
        }
    }

    return b76pipe_main(reverse, infile, outfile, message);
}
