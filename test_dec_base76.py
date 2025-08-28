#!/usr/bin/python
# test_dec_base76.py
"""
Round‑trip validation for Decimal ↔ Base76 conversions.
Covers normal values, boundaries, and a few big integers.
"""

from dec2base76 import dec_to_base76
from base76todec import base76_to_dec  # renamed for import‑safe module name

def test_dec_roundtrip():
    test_values = [
        0, 1, 75, 76,     # boundary cases
        12345, 987654321, # random large values
        10**12            # very large integer edge case
    ]
    for num in test_values:
        encoded = dec_to_base76(num)
        decoded = base76_to_dec(encoded)

        assert decoded == num, \
            f"Mismatch: {num} -> {encoded} -> {decoded}"
        print(f"✅ {num} → {encoded} → {decoded}")

if __name__ == "__main__":
    test_dec_roundtrip()
    print("All Decimal ↔ Base76 tests passed.")
