#!/usr/bin/python
## 76base2dec
# Takes a Base76 input and returns a decimal string (uppercase, no leading 0x)
# Source Code produced by Willtech 2023
# v0.2 with input validation

#Setup
from base76_alphabet import BASE76_LOOKUP
import sys

def validate_base76_input(s: str) -> str:
    """Ensure input is non-empty and contains only valid Base76 chars."""
    if not s:
        sys.exit("Error: No input provided.")
    invalid_chars = [ch for ch in s if ch not in BASE76_LOOKUP]
    if invalid_chars:
        sys.exit(f"Error: Invalid character(s) found: {''.join(invalid_chars)}")
    return s

def base76_to_dec(s: str) -> int:
    """Convert a validated Base76 string to decimal."""
    value = 0
    for char in s:
        value = value * 76 + BASE76_LOOKUP[char]
    return value

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("Usage: ./76base2dec <Base76_value>")
    user_input = validate_base76_input(sys.argv[1])
    decimal_value = base76_to_dec(user_input)
    print(decimal_value)
