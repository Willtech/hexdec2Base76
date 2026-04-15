# b76pipe — Compiled Base76 Streaming Pipeline (Single Binary)

`b76pipe` is a compiled, streaming‑capable Base76 encoder/decoder designed as a high‑performance companion to the **hexdec2Base76** project. It provides a reversible byte‑to‑Base76 pipeline, supports custom alphabets, and operates without loading the entire input into memory. This makes it suitable for large files, embedded systems, and constrained environments where predictable behaviour and 7‑bit safety are essential.

---

## Credits

**Source Code produced by **Graduate. Damian Williamson**  
**Willtech** Swan Hill, Victoria, Australia  

**Compiled pipeline variant based on the hexdec2Base76 project**  

**Converted & annotated with reference header by**  
Professor. Damian A. James Williamson Grad. + Copilot  

---

## Features

- **Single binary:** `b76pipe`
- **Streaming pipeline:**  
  - Forward: bytes → Base76 tokens  
  - Reverse: Base76 tokens → bytes  
  - No full‑file buffering; processes arbitrarily large inputs
- **Input options:**  
  - `-i infile` — read from file  
  - `message` argument — used when `-i` is not provided
- **Output options:**  
  - `-o outfile` — write to file  
  - default: stdout
- **Alphabet control:**  
  - Default alphabet from `base76_alphabet.h`  
  - Override with `--alpha alphabet_file`
- **7‑bit safe:**  
  - Only ASCII `< 128` characters used in the alphabet
- **CLI support:**  
  - `-h`, `--help`  
  - `--version`  
  - `-r` for reverse mode

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
./b76pipe "Hello"
```

### Forward pipeline from file

```sh
./b76pipe -i input.bin
```

### Forward pipeline to file

```sh
./b76pipe -i input.bin -o output.b76
```

### Reverse pipeline (Base76 → bytes)

```sh
./b76pipe -r "A1 B2 C3"
```

### Reverse pipeline from file

```sh
./b76pipe -r -i encoded.b76 -o decoded.bin
```

---

## Streaming Behaviour

`b76pipe` processes data incrementally:

- **Forward mode:** reads one byte at a time, emits one Base76 token at a time  
- **Reverse mode:** reads one token at a time, emits one byte at a time  

Because it never loads the entire input into memory, it can process **arbitrarily large inputs**, limited only by disk and I/O bandwidth.

---

## Custom Alphabet (`--alpha`)

You can override the default Base76 alphabet:

```sh
./b76pipe --alpha my_alphabet.txt "Hello"
```

### Alphabet file rules

The alphabet file may contain arbitrary content. The program constructs the alphabet as follows:

- Only characters with ASCII value `< 128` are considered (7‑bit safe)
- Whitespace is ignored
- Duplicate characters are ignored
- The first **76 unique** 7‑bit characters encountered form the alphabet
- If fewer than 76 unique characters are found, the program exits with an error

Example:

```
0123456789
ABCDEFGHIJKLMNOPQRSTUVWXYZ
abcdefghijklmnopqrstuvwxyz
!@#$%^&*()_+[]
```

Whitespace and line breaks are ignored.

---

## Exit Codes

`b76pipe` returns well‑defined exit codes for scripting and automation:

| Code | Meaning |
|------|---------|
| **0** | Success |
| **1** | General usage error (bad flags, missing args, file open failure) |
| **2** | Alphabet file error (invalid, unreadable, insufficient characters) |
| **3** | Invalid Base76 token during reverse mode |
| **4** | Token too long (exceeds internal buffer) |
| **5** | Internal pipeline error |
| **6** | Temporary stream creation failure |
| **7** | Output write failure (I/O error, disk full, etc.) |

---

## Version

`b76pipe` version **1.0.0.2026.04.15**

---
