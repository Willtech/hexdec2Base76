/*
===============================================================================
 hex2base76.c
 Takes a HEX input and returns Base76 (or reverse with -r)
 
 Source Code produced by Willtech 2023
 C translation v1.0 — based on validated Python implementation
 Converted & annotated with reference header by
 Professor. Damian A. James Williamson Grad. + Copilot

 Usage:
   ./hex2base76 <HEX_value>
   ./hex2base76 -r <base76_value>  // reverse: Base76 → HEX
===============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "base76_alphabet.h"

#define BASE76_LEN 76

int char_to_val(char c) {
    const char *p = strchr(BASE76_ALPHABET, c);
    return p ? (int)(p - BASE76_ALPHABET) : -1;
}

void validate_hex(const char *s) {
    if (*s == '\0') {
        fprintf(stderr, "Error: No input provided.\n");
        exit(1);
    }
    for (const char *p = s; *p; p++) {
        if (!isxdigit((unsigned char)*p)) {
            fprintf(stderr, "Error: Invalid HEX digit '%c'.\n", *p);
            exit(1);
        }
    }
}

void validate_base76(const char *s) {
    if (*s == '\0') {
        fprintf(stderr, "Error: No input provided.\n");
        exit(1);
    }
    for (const char *p = s; *p; p++) {
        if (char_to_val(*p) == -1) {
            fprintf(stderr, "Error: Invalid Base76 character '%c'.\n", *p);
            exit(1);
        }
    }
}

char *dec_to_base76(unsigned long long n) {
    static char buf[128];
    int i = 0;
    if (n == 0) {
        buf[i++] = BASE76_ALPHABET[0];
        buf[i] = '\0';
        return buf;
    }
    while (n > 0) {
        buf[i++] = BASE76_ALPHABET[n % BASE76_LEN];
        n /= BASE76_LEN;
    }
    buf[i] = '\0';
    for (int j = 0; j < i/2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i-1-j];
        buf[i-1-j] = tmp;
    }
    return buf;
}

unsigned long long base76_to_dec(const char *s) {
    unsigned long long val = 0;
    while (*s) {
        int digit = char_to_val(*s++);
        val = val * BASE76_LEN + digit;
    }
    return val;
}

int main(int argc, char *argv[]) {
    int reverse = 0;
    const char *value = NULL;

    if (argc == 3 && strcmp(argv[1], "-r") == 0) {
        reverse = 1;
        value = argv[2];
    } else if (argc == 2) {
        value = argv[1];
    } else {
        fprintf(stderr, "Usage: %s [-r] <value>\n", argv[0]);
        return 1;
    }

    if (reverse) {
        validate_base76(value);
        unsigned long long dec = base76_to_dec(value);
        printf("%llX\n", dec);
    } else {
        validate_hex(value);
        unsigned long long dec = strtoull(value, NULL, 16);
        printf("%s\n", dec_to_base76(dec));
    }

    return 0;
}
