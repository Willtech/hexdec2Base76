# b76pipe — Base76 7‑bit Streaming Pipeline Encoder/Decoder

`b76pipe` is a compiled Base76 encoder/decoder implementing the reversible
7‑bit binary pipeline used in the **hexdec2Base76** project.  
This version operates in a **file‑stream topology**: input is read in chunks,
Stage‑1 output (if enabled) is emitted immediately to **stderr**, and the final
Stage‑2 output is written only once at the end.

Although input is streamed, the Base76 pipeline requires maintaining a growing
big‑integer representation of the entire message. Therefore, memory usage
increases with input size.

---

## Credits

**Source Code:** Graduate. Damian Williamson  
**Willtech**, Swan Hill, Victoria, Australia  

**Compiled pipeline variant based on the hexdec2Base76 project**  

**Design, commentary & integration sketch:**  
Professor. Damian A. James Williamson Grad. + Copilot  

---

## Features

- **Single binary:** `b76pipe`
- **Streaming input topology**
  - Reads input in chunks (stdin or file)
  - Stage‑1 output (`-v`) printed immediately to **stderr**
  - Stage‑2 output printed only once at the end
- **Forward mode:** bytes → Base76 (7‑bit pipeline)
- **Reverse mode:** Base76 → bytes
- **Input options:**
  - `-i infile` — read from file (or `-` for stdin)
  - message arguments when `-i` is not used
- **Output options:**
  - `-o outfile` — write final Stage‑2 output to file
  - default: stdout
- **Diagnostic options:**
  - `-v` — print Stage‑1 tokens to **stderr**
  - `--file-stream` — accepted for compatibility (no behavioural change)
- **Help/version:**
  - `-h`, `--help`
  - `--version`

---

## Building

```sh
make
```

Produces:

```
b76pipe
```

Clean build artifacts:

```sh
make clean
```

---

## Usage

### Forward pipeline (bytes → Base76)

```sh
./b76pipe "Hello world"
```

### Forward pipeline from file

```sh
./b76pipe -i input.bin
```

### Forward pipeline with Stage‑1 output

```sh
./b76pipe -v -i input.bin
```

(Stage‑1 tokens appear on **stderr**, Stage‑2 on **stdout**)

### Reverse pipeline (Base76 → bytes)

```sh
./b76pipe -r -i encoded.b76 -o decoded.bin
```

---

## Streaming Behaviour

`b76pipe` reads input incrementally in chunks.  
However, due to the Base76 big‑integer pipeline, the program must maintain a
growing big‑integer representation of the entire message.

**This means:**

- Input is streamed  
- Stage‑1 output is streamed  
- **But memory usage grows with input size**

This is required for exact reversibility of the Base76 7‑bit pipeline.

---

## Exit Codes

`b76pipe` returns:

| Code | Meaning |
|------|---------|
| **0** | Success |
| **1** | Usage error, file I/O error, or invalid Base76 input |

---

## Version

`b76pipe` version **1.0.0.2026.04.15**
