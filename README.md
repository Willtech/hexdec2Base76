# Base76 Conversion Utilities

> Author: Willtech  
> C Translation: Professor. Damian A. James Williamson Grad. + Copilot  
> Version: v1.0  
> License: MIT  
> Languages: C (compiled), Python (reference)  
> Purpose: Bidirectional conversion between Decimal, Hexadecimal, and Base76 using a fixed 76-character alphabet.

---

📦 Project Overview

This repository contains a set of command-line utilities for converting between:

- Decimal ↔️ Base76  
- Hexadecimal ↔️ Base76  

The utilities are implemented in both Python (for validation and prototyping) and C (for performance and portability). Each converter supports a -r flag to reverse the conversion direction.

The Base76 alphabet is fixed and shared across all implementations:

```
0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()_+[]
```

---

⚙️ Features

| Feature                     | Description                                                                 |
|-----------------------------|-----------------------------------------------------------------------------|
| 🔁 Bidirectional Conversion | Use -r to reverse direction (e.g., Base76 → Decimal)                      |
| 🔒 Input Validation         | Rejects malformed input with clear error messages                          |
| 🧱 Shared Alphabet          | Defined in base76alphabet.h and base76alphabet.py                     |
| 🚀 Portable C Code          | Compiles with GCC/Clang on Linux, macOS, Windows (via MinGW)                |
| 🧪 Python Test Harness      | Validates round-trip accuracy for all conversions                          |
| 📜 Makefile Build System    | One command builds both binaries                                            |
| 🧩 Modular Design           | Easy to extend to other bases or formats                                    |

---

🧠 Architecture Diagram

`plaintext
+------------------+      +-------------------+      +-------------------+
|  dec2base76.c    | ---> | base76alphabet.h | ---> | decto_base76()   |
|   - main()       |      | shared alphabet   |      | base76todec()   |
+------------------+      +-------------------+      +-------------------+

+------------------+      +-------------------+      +-------------------+
|  hex2base76.c    | ---> | base76alphabet.h | ---> | hexto_base76()   |
|   - main()       |      |                   |      | base76tohex()   |
+------------------+      +-------------------+      +-------------------+
`

---

🛠️ Compilation Instructions

Build with Makefile

```bash
make all     # Builds dec2base76 and hex2base76
make clean   # Removes compiled binaries
```

Output Binaries

```
./dec2base76
./hex2base76
```

---

🚀 Usage Examples

Decimal ↔️ Base76

```bash
./dec2base76 12345         # Decimal → Base76
./dec2base76 -r 3d!        # Base76 → Decimal
```

Hex ↔️ Base76

```bash
./hex2base76 DEADBEEF      # HEX → Base76
./hex2base76 -r K#Q$%      # Base76 → HEX
```

---

🔤 Full Base76 Alphabet Table

| Index | Char | ASCII Dec | ASCII Hex |
|-------|------|-----------|-----------|
| 0     | 0    | 48        | 0x30      |
| 1     | 1    | 49        | 0x31      |
| 2     | 2    | 50        | 0x32      |
| ...   | ...  | ...       | ...       |
| 75    | ]    | 93        | 0x5D      |

> The full table includes digits, uppercase, lowercase, and 14 symbols.  
> See base76alphabet.h or base76alphabet.py for the canonical definition.

---

✅ Input Validation Rules

| Converter      | Allowed Characters                            |
|----------------|-----------------------------------------------|
| Decimal        | 0–9                                       |
| Hexadecimal    | 0–9, A–F, a–f                      |
| Base76         | 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()_+[] |

---

⚠️ Error Messages

| Condition               | Message                                        |
|-------------------------|------------------------------------------------|
| No input                | Error: No input provided.                    |
| Invalid decimal digit   | Error: Invalid decimal digit '<c>'.          |
| Invalid HEX digit       | Error: Invalid HEX digit '<c>'.              |
| Invalid Base76 char     | Error: Invalid Base76 character '<c>'.       |

---

🧮 Algorithm Breakdown

Decimal → Base76

```c
while (n > 0) {
    digit = n % 76;
    n /= 76;
    append(BASE76_ALPHABET[digit]);
}
reverse(buffer);
```

Base76 → Decimal

```c
val = 0;
for each char:
    val = val * 76 + chartoindex(char);
```

Hex → Base76

```c
decimal = strtoull(hex_str, NULL, 16);
base76 = dectobase76(decimal);
```

Base76 → Hex

```c
decimal = base76todec(base76_str);
hex_str = format(decimal, "%llX");
```

---

🔁 ASCII Flow Diagrams

```plaintext
[Decimal] --dectobase76()--> [Base76] --base76todec()--> [Decimal]

[HEX] ----hextobase76()-----> [Base76] --base76tohex()--> [HEX]
```

---

🧪 Python Test Harness

Run Test

```bash
cp ./tests/* ./python/ 
python3 test_dec_base76.py
python3 test_hex_base76.py
```

Expected Output

```
✅ 12345 → 3d! → 12345
✅ DEADBEEF → K#Q$% → DEADBEEF
All tests passed.
```

---

📁 File Structure

```plaintext
project/
├── base76_alphabet.h       # C header for Base76 alphabet
├── dec2base76.c            # Decimal ↔️ Base76 converter
├── hex2base76.c            # Hex ↔️ Base76 converter
├── Makefile                # Build script
├── python/                 # Python reference implementations
│   ├── base76_alphabet.py
│   ├── dec2base76.py
│   ├── base76todec.py
│   ├── hex2base76.py
│   └── base76tohex.py
└── tests/
    ├── test_dec_base76.py
    └── test_hex_base76.py
```

---

📜 Change Log

| Version | Date       | Change                                |
|---------|------------|---------------------------------------|
| 1.0     | 2025-08-29 | Initial C translation & Makefile build|

---

👥 Maintainers

- Willtech — Original Python implementation  
- Professor. Damian A. James Williamson Grad. — C translation, validation, and artifact design  
- Copilot — Documentation, refactoring, and test harness integration  

---

📄 License

This project is licensed under the MIT License.  
See LICENSE for details.

---
