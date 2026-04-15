/*
===============================================================================
 b76pipe.h
 Header for the compiled Base76 streaming pipeline.

 Source Code produced by Willtech 2023–2026
 Compiled pipeline variant based on hexdec2Base76 project
 Converted & annotated with reference header by
 Professor. Damian A. James Williamson Grad. + Copilot
===============================================================================
*/

#ifndef B76PIPE_H
#define B76PIPE_H

#define TOOL_NAME    "b76pipe"
#define TOOL_VERSION "1.0.0.2026.04.15"

#define BASE76_LEN 76

/* Active alphabet buffer (populated from base76_alphabet.h or --alpha) */
extern char active_alphabet[BASE76_LEN + 1];

/* Alphabet loader */
int load_alphabet_file(const char *path);

/* Pipeline operations */
int pipeline_forward(FILE *in, FILE *out);
int pipeline_reverse(FILE *in, FILE *out);

/* Main wrapper */
int b76pipe_main(int reverse,
                 const char *infile,
                 const char *outfile,
                 const char *message);

#endif
