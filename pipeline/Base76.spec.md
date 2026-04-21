Base76 Specification

Version: 1.0  
Scope: This document specifies the Base76 encoding used by the Willtech Base76 Pipeline Suite, including b76pipe_stream. It defines the alphabet, encoding and decoding rules, bit‑level layout, and behavioural guarantees required for interoperable implementations.

---

1. Overview

Base76 is a reversible binary‑to‑text encoding that maps arbitrary byte streams to a restricted 7‑bit character alphabet of length 76. It is designed for:

- Deterministic, lossless round‑trip conversion.
- Streaming operation (incremental encode/decode).
- Compatibility with 7‑bit clean channels.
- Explicit, verifiable alphabet semantics.

The encoding is conceptually split into two stages:

1. Stage‑1: Binary → Base76 digits (integer representation).
2. Stage‑2: Base76 digits → 7‑bit character stream (alphabet mapping).

Decoding reverses these stages in the opposite order.

---

2. Alphabet

2.1 Default Alphabet

The default alphabet is defined in base76_alphabet.h as:

- A constant array BASE76_ALPHABET[76].
- A length constant BASE76ALPHABETLEN == 76.

The alphabet:

- Contains exactly 76 distinct characters.
- Uses only 7‑bit characters (0x00–0x7F).
- Contains no duplicates.

The exact character set is an implementation detail but MUST be consistent across all tools that claim compatibility with this specification.

2.2 Custom Alphabet

A custom alphabet may be provided as a text file. The loader:

1. Reads the file as bytes.
2. Ignores all whitespace characters (space, tab, newline, carriage return, form feed, vertical tab).
3. Ignores any byte with value ≥ 128.
4. Collects the first 76 unique remaining characters in order of appearance.

If fewer than 76 unique valid characters are found, the alphabet load MUST fail.

Constraints:

- The resulting alphabet MUST contain exactly 76 unique 7‑bit characters.
- The position of each character in the alphabet defines its digit value:

  - Index 0 → digit value 0
  - Index 1 → digit value 1
  - …
  - Index 75 → digit value 75

Implementations MUST treat any character not in the active alphabet as invalid during decoding.

---

3. Encoding

3.1 High‑Level Process

Given an input byte stream \(B\):

1. Interpret \(B\) as a non‑negative integer \(N\) in base‑256 (big‑endian).
2. Convert \(N\) to a sequence of Base76 digits \(D\) (Stage‑1).
3. Map each digit in \(D\) to its corresponding alphabet character (Stage‑2).
4. Emit the resulting character stream as the encoded output.

The process MUST be deterministic and fully reversible.

3.2 Stage‑1: Binary to Base76 Digits

3.2.1 Integer Interpretation

The input byte sequence \(B = b0, b1, \dots, b_{k-1}\) is interpreted as:

\[
N = \sum{i=0}^{k-1} bi \cdot 256^{k-1-i}
\]

where each \(b_i\) is in \([0, 255]\).

3.2.2 Base76 Conversion

To obtain the digit sequence \(D = d0, d1, \dots, d_{m-1}\) (most significant digit first):

1. If \(N = 0\) and the input length is zero, the digit sequence is empty.
2. Otherwise, repeatedly divide \(N\) by \(76\):

   - At each step:  
     \[
     N = 76 \cdot Q + R
     \]
     where \(Q\) is the quotient and \(R\) is the remainder.
   - Append \(R\) to a temporary list of digits.
   - Replace \(N\) with \(Q\).
   - Stop when \(N = 0\).

3. Reverse the temporary list of remainders to obtain \(D\).

Each digit \(di\) satisfies \(0 \le di < 76\).

3.3 Stage‑2: Digits to Characters

Each digit \(d_i\) is mapped to a character:

\[
ci = \text{alphabet}[di]
\]

The encoded output is the concatenation of all \(c_i\) in order.

---

4. Decoding

4.1 High‑Level Process

Given an encoded character stream \(C\):

1. Map each character to its digit value using the active alphabet.
2. Interpret the digit sequence as a base‑76 integer \(N\).
3. Convert \(N\) back to a byte sequence in base‑256.
4. Emit the resulting bytes as the decoded output.

4.2 Stage‑2 Reverse: Characters to Digits

For each character \(c_i\) in the input:

- If \(ci\) is whitespace, it MAY be ignored (implementation‑defined; b76pipestream ignores whitespace).
- Otherwise, find its index in the active alphabet:
  - If found at index \(j\), the digit value is \(d_i = j\).
  - If not found, decoding MUST fail with an error.

The result is a digit sequence \(D = d0, d1, \dots, d_{m-1}\).

4.3 Stage‑1 Reverse: Base76 Digits to Binary

Interpret the digit sequence \(D\) as:

\[
N = \sum{i=0}^{m-1} di \cdot 76^{m-1-i}
\]

Then convert \(N\) to a byte sequence in base‑256:

1. If \(N = 0\) and there were no digits, the output is an empty byte sequence.
2. Otherwise, repeatedly divide \(N\) by \(256\):

   - At each step:  
     \[
     N = 256 \cdot Q + R
     \]
     where \(Q\) is the quotient and \(R\) is the remainder.
   - Append \(R\) to a temporary list of bytes.
   - Replace \(N\) with \(Q\).
   - Stop when \(N = 0\).

3. Reverse the temporary list of remainders to obtain the original byte sequence.

---

5. Streaming Behaviour

Although the mathematical description above is expressed in terms of a single integer \(N\), b76pipe_stream implements a streaming variant:

- Input bytes are consumed incrementally.
- Internal big‑integer state is updated as data arrives.
- Base76 digits and characters are emitted as soon as they become determined.
- Decoding similarly reconstructs bytes incrementally from the digit stream.

Any conforming streaming implementation MUST:

- Produce the same final output as the non‑streaming integer model.
- Be deterministic for a given input and alphabet.
- Not introduce padding or framing beyond what is implied by the digit sequence.

---

6. Error Handling

A conforming implementation MUST treat the following as errors:

- Invalid alphabet file:
  - Fewer than 76 unique 7‑bit characters after filtering.
- Invalid encoded input:
  - Character not present in the active alphabet (excluding ignored whitespace).
- Internal arithmetic failure:
  - Big‑integer operations that overflow implementation limits (if any) SHOULD fail cleanly.

On error, the implementation MUST:

- Stop processing.
- Not emit partial decoded data beyond what is already committed.
- Return a non‑zero exit status (for CLI tools).

---

7. Self‑Test Requirements

While not strictly part of the mathematical encoding, the reference implementation (b76pipe_stream) defines a self‑test behaviour that other tools MAY emulate:

- Self‑test MUST:
  - Encode a known input using the forward pipeline.
  - Decode the result using the reverse pipeline.
  - Compare the decoded bytes to the original.
- Self‑test MUST:
  - Fail if any mismatch occurs.
  - Exit immediately after completion (success or failure).

Self‑test is RECOMMENDED for:

- Verifying new implementations.
- Regression testing.
- Alphabet changes.

---

8. Determinism and Interoperability

To be considered Base76‑compatible under this specification, an implementation MUST:

1. Use an alphabet of exactly 76 unique 7‑bit characters.
2. Map digit values to alphabet indices as defined in Section 2.
3. Implement encoding and decoding as defined in Sections 3 and 4.
4. Treat invalid characters and invalid alphabets as errors.
5. Produce bit‑for‑bit identical decoded output for any valid encoded input produced by another conforming implementation using the same alphabet.

Streaming vs non‑streaming implementations are both valid, but they MUST agree on final outputs.

---

9. Security and Robustness Considerations

- Implementations SHOULD:
  - Validate all input characters against the active alphabet.
  - Reject malformed alphabets early.
  - Avoid unbounded memory growth for extremely large inputs (e.g., via chunked processing).
- Implementations MUST NOT:
  - Silently substitute characters not in the alphabet.
  - Implicitly change alphabets mid‑stream.

---

10. Reference Implementation

The canonical reference for this specification is:

- b76pipe_stream.c
- base76_alphabet.h

from the Willtech hexdec2Base76 repository, pipeline subproject.

These files embody:

- The default alphabet.
- The streaming big‑integer implementation.
- The forward and reverse pipelines.
- The self‑test behaviour.

Any divergence from the rules in this document SHOULD be considered an implementation detail, not a change to the Base76 format itself.
