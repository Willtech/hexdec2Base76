### b76pipe_stream

**Streaming Base76 7‑bit binary pipeline encoder/decoder**

A streaming, character‑by‑character variant of `b76pipe` that emits output continuously as input is processed. Designed for pipelines and long‑running streams while preserving the original CLI and Stage‑1 token semantics where possible.

---

### Features

- **Forward (encode)**: per‑byte Stage‑1 tokens printed to **stderr** when `-v` is used; final encoded output streamed to **stdout** or `-o` file as bytes arrive.  
- **Reverse (decode)**: consumes Base76 characters continuously, reconstructs bits and Base76 digits, and emits decoded bytes as soon as token boundaries are resolved.  
- **CLI parity**: supports `-r`, `-i infile`, `-o outfile`, `-v`, `--version`, `--help`.  
- **Streaming friendly**: intended for interactive and streaming workflows (output appears without waiting for full input).

---

### Build

Requires a C11 compiler (gcc/clang) and the repository headers `b76pipe.h` and `base76_alphabet.h`.

```bash
make
# or
gcc -std=c11 -O2 -Wall -Wextra -o b76pipe_stream b76pipe_stream.c
```

---

### Usage

```bash
# Encode (forward)
./b76pipe_stream -i infile -o outfile

# Decode (reverse)
./b76pipe_stream -r -i infile.b76 -o outfile

# Read from stdin and write to stdout
cat file | ./b76pipe_stream

# Print Stage 1 tokens (per-byte Base76) to stderr
./b76pipe_stream -v -i infile
```

**Notes**
- Forward streaming processes input per‑byte; for exact bit‑for‑bit parity with the original global big‑int encoding, use the non‑streaming variant that buffers the entire input before encoding.

---

### Troubleshooting

- **Invalid Base76 character**: ensure the input uses the same `BASE76_ALPHABET` as the binary (check `base76_alphabet.h`).  
- **Binary corruption when transporting via text (CSV/editors)**: encode binary (Base64/hex) before transport and preserve length/checksum; decode back to binary before restoring files.  
- **Decoded file not recognized (e.g., PNG header missing)**: compare checksums of original and restored files; check that restore writes in binary mode (`wb`).

---

### Credits

**Author:** Graduate. Damian Williamson — Willtech, Swan Hill, Victoria, Australia.  
**AI Collaboration:** Microsoft Copilot — pipeline design, commentary, and integration sketch.

---

### License

```
MIT License

Copyright (c) 2026 Willtech

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
