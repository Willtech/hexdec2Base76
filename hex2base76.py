#!/usr/bin/python
## hex2base76
# Takes a HEX input and returns Base76
# Source Code produced by Willtech 2023
# v0.2 with input validation

import sys
from base76_alphabet import BASE76_ALPHABET

def validate_hex_input(s: str) -> str:
    """Ensure input is non-empty and contains only valid hex digits."""
    if not s:
        sys.exit("Error: No input provided.")
    # Allow 0-9, A-F, a-f
    valid_chars = "0123456789abcdefABCDEF"
    invalid_chars = [ch for ch in s if ch not in valid_chars]
    if invalid_chars:
        sys.exit(f"Error: Invalid HEX character(s) found: {''.join(invalid_chars)}")
    return s

def hex_to_base76(hex_str: str) -> str:
    """Convert a validated hex string (uppercase/lowercase allowed) to Base76."""
    decimal_value = int(hex_str, 16)

    if decimal_value == 0:
        return BASE76_ALPHABET[0]

    encoded_digits = []
    while decimal_value > 0:
        decimal_value, remainder = divmod(decimal_value, 76)
        encoded_digits.append(BASE76_ALPHABET[remainder])

    return ''.join(reversed(encoded_digits))

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("Usage: ./hex2base76 <HEX_value>")
    user_input = validate_hex_input(sys.argv[1])
    print(hex_to_base76(user_input))
