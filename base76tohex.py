#!/usr/bin/python
## 76base2hex
# Takes a Base76 input and returns a HEX string (uppercase, no leading 0x)
# Source Code produced by Willtech 2023
# v0.2 with input validation

#Setup
from base76_alphabet import BASE76_ALPHABET, BASE76_LOOKUP
import sys

def validate_base76_input(s: str) -> str:
    """Ensure input is non-empty and contains only valid Base76 chars."""
    if not s:
        sys.exit("Error: No input provided.")
    invalid_chars = [ch for ch in s if ch not in BASE76_LOOKUP]
    if invalid_chars:
        sys.exit(f"Error: Invalid character(s) found: {''.join(invalid_chars)}")
    return s

def base76_to_hex(base76_str: str) -> str:
    """Convert a validated Base76 string back to a HEX string (uppercase, no leading 0x)."""
    decimal_value = 0
    for ch in base76_str:
        decimal_value = decimal_value * 76 + BASE76_LOOKUP[ch]
    return format(decimal_value, 'X')

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("Usage: ./76base2hex <Base76_value>")
    user_input = validate_base76_input(sys.argv[1])
    print(base76_to_hex(user_input))
