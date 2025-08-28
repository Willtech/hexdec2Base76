#!/usr/bin/python
## dec2base76
# Takes a decimal input and returns Base76
# Source Code produced by Willtech 2023
# v0.2 with input validation

import sys
from base76_alphabet import BASE76_ALPHABET

def validate_decimal_input(s: str) -> int:
    """Ensure input is a non-negative integer and return it as int."""
    if not s:
        sys.exit("Error: No input provided.")
    if not s.isdigit():
        sys.exit(f"Error: Invalid decimal number '{s}'. Must be digits only.")
    return int(s)

def dec_to_base76(n: int) -> str:
    """Convert a validated decimal integer to Base76."""
    if n == 0:
        return BASE76_ALPHABET[0]
    digits = []
    while n > 0:
        n, rem = divmod(n, 76)
        digits.append(BASE76_ALPHABET[rem])
    return ''.join(reversed(digits))

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("Usage: ./dec2base76 <decimal_value>")
    decimal_value = validate_decimal_input(sys.argv[1])
    print(dec_to_base76(decimal_value))
