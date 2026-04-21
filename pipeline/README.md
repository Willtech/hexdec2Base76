# b76pipe_stream

**Streaming Base76 7‑bit binary pipeline encoder/decoder**

`b76pipe_stream` is the streaming variant of the Willtech Base76 Pipeline Suite.  
It performs fully reversible Base76 encoding/decoding using a strict 76‑character alphabet, deterministic forward/reverse semantics, and a comprehensive internal self‑test facility.  
Unlike the non‑streaming `b76pipe`, this tool processes data incrementally and emits output continuously, making it ideal for pipelines, FIFOs, device files, and long‑running streams.

---

## Features

### Forward Encoding
- Converts raw bytes → Base76 Stage‑1 digits → 7‑bit bitstream → Base76 Stage‑2 output.
- Stage‑1 Base76 tokens printed to **stderr** when `-v` is used.
- Stage‑2 encoded output streamed to **stdout** or to a file via `-o`.

### Reverse Decoding
- Consumes Base76 Stage‑2 characters continuously.
- Reconstructs the 7‑bit bitstream and Stage‑1 digits.
- Emits decoded bytes as soon as boundaries resolve.
- Whitespace in encoded input is ignored.

### Alphabet Handling
- Default alphabet defined in `base76_alphabet.h`.
- Custom alphabet via `--alpha <file>`:
  - First 76 unique 7‑bit characters are used.
  - Whitespace ignored.
  - Duplicates ignored.
  - Characters ≥ 128 ignored.
  - Error if fewer than 76 unique characters.

### Self‑Test Facility
`--selftest` provides deterministic forward/reverse validation:

| Mode | Behaviour |
|------|-----------|
| `--selftest` | Use built‑in diagnostic message and exit. |
| `--selftest "string"` | Use provided string and exit. |
| `--selftest -i file` | Use contents of `<file>` and exit. |
| `--selftest -i file -o file` | Run full selftest, then forward‑encode `<file>` to `<outfile>`, reverse‑decode it, compare to original, then exit. |

Selftest always writes:
- `.b76pipe_stream.selftest.i` — original input  
- `.b76pipe_stream.selftest.o` — encoded Stage‑2 output  

Selftest **always exits** after completion.

---

## Build and Test

Clone the repository:

```sh
git clone git@github.com:Willtech/hexdec2Base76.git
cd hexdec2Base76/pipeline
```

Build the streaming encoder/decoder:

```sh
make
```

Run the full Base76 pipeline test suite:

```sh
make test
```

Install the binary and manpages on OpenBSD:

```sh
doas make install
```

After installation, the manpages are available:

Run:
```sh
doas makewhatis
```

```sh
man 3 b76pipe_stream
man 3 base76_alphabet
```

---

## Usage

### Forward Encoding

```sh
./b76pipe_stream -i infile -o outfile
```

### Reverse Decoding

```sh
./b76pipe_stream -r -i infile.b76 -o outfile
```

### Streaming via stdin/stdout

```sh
cat file | ./b76pipe_stream
```

### Verbose Stage‑1 Tokens

```sh
./b76pipe_stream -v -i infile
```

### Custom Alphabet

```sh
./b76pipe_stream --alpha alphabet.txt -i infile
```

### Self‑Test Examples

```sh
./b76pipe_stream --selftest
./b76pipe_stream --selftest "HelloWorld123"
./b76pipe_stream --selftest -i message.bin
./b76pipe_stream --selftest -i message.bin -o encoded.txt
```

---

## Troubleshooting

- **Invalid Base76 character**  
  Ensure the input uses the same alphabet as the program (default or custom).

- **Decoded file corrupted**  
  Compare checksums of original vs decoded.  
  Ensure output is written in binary mode (`wb`).

- **Custom alphabet rejected**  
  Check that the file contains at least 76 unique 7‑bit characters.

---

## Credits

**Design, architecture, reference implementation:**  
Professor Damian A. James Williamson Grad.  
Willtech, Swan Hill, Victoria, Australia  

**Implementation, documentation, diagnostic tooling:**  
Professor Damian A. James Williamson Grad. + Copilot  

Part of the **Willtech Base76 Pipeline Suite**

---

### License

```
Willtech Open Technical Artifact License (WOTAL)  
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

