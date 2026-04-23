# Willtech Base76 Pipeline Suite  
### Streaming‑Friendly 7‑Bit Reversible Base76 Encoding

The Willtech Base76 Pipeline Suite provides a **fully reversible**, **stream‑safe**, **7‑bit clean** encoding pipeline designed for deterministic forward/reverse transformations of arbitrary binary data.

<p align="center">

  <!-- Build Status -->
  <img src="https://img.shields.io/badge/build-passing-brightgreen.svg" alt="Build Status">

  <!-- Test Suite -->
  <img src="https://img.shields.io/badge/tests-100%25%20passing-brightgreen.svg" alt="Test Status">

  <!-- Pipeline Spec -->
  <img src="https://img.shields.io/badge/Base76%20Pipeline-Spec%201.0-blue.svg" alt="Pipeline Spec">

  <!-- Stage‑1 Support -->
  <img src="https://img.shields.io/badge/Stage--1-supported-blueviolet.svg" alt="Stage‑1 Support">

  <!-- Stage‑2 Support -->
  <img src="https://img.shields.io/badge/Stage--2-7bit%20clean-orange.svg" alt="Stage‑2 Support">

  <!-- DeepWiki -->
  <a href="https://deepwiki.com/Willtech/hexdec2Base76">
    <img src="https://deepwiki.com/badge.svg" alt="Ask DeepWiki" />
  </a>

  <!-- Platform -->
  <img src="https://img.shields.io/badge/platform-OpenBSD%20%7C%20Linux%20%7C%20POSIX-lightgrey.svg" alt="Platform">

  <!-- Language -->
  <img src="https://img.shields.io/badge/language-C99-blue.svg" alt="Language">

  <!-- License -->
  <a href="https://github.com/Willtech/hexdec2Base76/tree/master/pipeline#license">
    <img src="https://img.shields.io/badge/license-WOTAL-green.svg" alt="License">
  </a>
    <img src="https://img.shields.io/badge/license-MIT-blue" alt="MIT License" />
  </a>
  
  <!-- Version -->
  <img src="https://img.shields.io/badge/version-1.0.0.2026.04.15-yellow.svg" alt="Version">

</p>


This repository contains the reference implementation:

```
b76pipe_stream
```

which performs:

- **Stage‑1**: Base76 tokenization of bytes  
- **Stage‑2**: Bit‑level Base76 encoding (48/49 mapping)  
- **Reverse decoding** back to bytes  
- **Internal self‑test** for correctness  
- **Custom alphabet loading**  
- **Streaming‑safe I/O**  

The design is documented in detail in  
**Issue #1: “Formal Definition of the Base76 Pipeline”**  
which establishes the canonical behaviour of Stage‑1 and Stage‑2.

---

## Overview

The Base76 pipeline consists of **two reversible stages**:

```
Bytes → Stage‑1 → Stage‑2
Stage‑2 → Stage‑1 → Bytes
```

Each stage is independently reversible and uses the same active Base76 alphabet.

---

### Stage‑1: Byte‑to‑Base76 Token Layer

Stage‑1 converts each byte (0–255) into a **Base76 number**, encoded using the active alphabet.

Example (default alphabet):

```
Byte: 84 ('T') → Stage‑1: "18"
Byte: 104 ('h') → Stage‑1: "1S"
Byte: 105 ('i') → Stage‑1: "1T"
Byte: 115 ('s') → Stage‑1: "1d"
```

Stage‑1 tokens are:

- Whitespace‑separated  
- Base76 numbers  
- Deterministic  
- Reversible  

Stage‑1 is used for:

- Human‑readable debugging (`-v`)  
- Stage‑1 input mode (`--stage1`)  
- Internal selftest diagnostics  

---

### Stage‑2: Bit‑Level Base76 Encoding

Stage‑2 converts each Stage‑1 Base76 digit into **7 bits**, then encodes each bit as:

- `'0'` → Base76 encoding of decimal **48**  
- `'1'` → Base76 encoding of decimal **49**

This produces a 7‑bit clean, streaming‑safe output suitable for:

- Text‑only channels  
- Logging  
- Transport through restricted systems  

Stage‑2 is the default forward output of `b76pipe_stream`.

---

### Reverse Pipeline

Reverse decoding performs:

```
Stage‑2 → bits → Stage‑1 → bytes
```

Additionally, when using:

```
-r --stage1
```

the program bypasses Stage‑2 and decodes Stage‑1 tokens directly:

```
Stage‑1 → bytes
```

This is useful when Stage‑1 tokens have already been extracted.

---

## Usage

```
b76pipe_stream [options] [message]
```

## Options

### I/O
```
-i <file>     Input file (default: stdin)
-o <file>     Output file (default: stdout)
```

### Modes
```
-r            Reverse decode (Stage‑2 → bytes)
-v            Verbose: print Stage‑1 tokens to stderr
--stage1      Treat input as Stage‑1 tokens (forward or reverse)
```

### Alphabet
```
--alpha <file>
    Load custom Base76 alphabet.
    Must contain at least 76 unique 7‑bit characters.
```

### Selftest
```
--selftest
--selftest "string"
--selftest -i <file>
--selftest -i <file> -o <file>
```

Selftest performs:

- Internal forward/reverse validation  
- File‑based forward/reverse validation  
- Writes `.b76pipe_stream.selftest.i` and `.o`  

---

## Examples

### Forward encode (stdin → stdout)

```
echo "Hello" | b76pipe_stream
```

### Forward encode with Stage‑1 input

```
b76pipe_stream --stage1 "18 1S 1T 1d"
```

### Reverse decode Stage‑2

```
b76pipe_stream -r -i encoded.txt
```

### Reverse decode Stage‑1 directly

```
b76pipe_stream -r --stage1 "18 1S 1T 1d"
```

### Verbose mode (show Stage‑1 tokens)

```
b76pipe_stream -v -i message.txt
```

### Custom alphabet

```
b76pipe_stream --alpha myalphabet.txt -i message.txt
```

---

## Stage‑1 Input Mode (`--stage1`)

### Forward mode (`--stage1`)

```
--stage1 "18 1S 1T 1d"
```

interprets the input as Stage‑1 tokens and converts them to bytes before Stage‑2 encoding.

### Reverse mode (`-r --stage1`)

```
-r --stage1 "18 1S 1T 1d"
```

decodes Stage‑1 tokens directly back to bytes.

---

## Selftest

Run built‑in selftest:

```
b76pipe_stream --selftest
```

Selftest validates:

- Stage‑1 correctness  
- Stage‑2 correctness  
- Internal forward/reverse  
- File‑based forward/reverse  

Success message:

```
b76pipe_stream self-test Okay.
```

---

## Alphabet Rules

A valid Base76 alphabet must:

- Contain **exactly 76 unique characters**
- All characters must be **7‑bit clean**
- Whitespace is ignored
- Duplicates are ignored
- Characters ≥ 128 are ignored

---

## Test Suite

The repository includes a comprehensive test suite validating:

- stdin/stdout  
- file I/O  
- Stage‑1 forward  
- Stage‑1 reverse  
- Stage‑2 forward  
- Stage‑2 reverse  
- verbose mode  
- custom alphabet  
- selftest modes  
- byte‑for‑byte reversibility  

Run:

```
cd tests
./test.sh
```

---

## License

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

---

## Status

This implementation is the **canonical reference** for the Base76 7‑bit reversible pipeline as defined in [Base76.spec.md](https://github.com/Willtech/hexdec2Base76/blob/master/pipeline/Base76.spec.md) [Issue #1](https://github.com/Willtech/hexdec2Base76/issues/1).
