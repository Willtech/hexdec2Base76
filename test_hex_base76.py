#!/usr/bin/python
# test_hex_base76.py
"""
Round‑trip test: HEX ↔ Base76
"""

from hex2base76 import hex_to_base76
from base76tohex import base76_to_hex

def test_hex_roundtrip():
    test_values = [
        "0", "A", "10", "1F4",
        "DEADBEEF", "FFFFFFFF",
        "0001",     # leading zeros edge case
        "abcdef"    # lowercase HEX edge case
    ]
    for hex_val in test_values:
        encoded = hex_to_base76(hex_val)
        decoded = base76_to_hex(encoded)

        # strip leading zeros for comparison unless the original was zero
        normalized_input = hex_val.upper().lstrip("0") or "0"
        normalized_output = decoded.upper().lstrip("0") or "0"

        assert normalized_output == normalized_input, \
            f"Mismatch: {hex_val} -> {encoded} -> {decoded}"
        print(f"✅ {hex_val} → {encoded} → {decoded}")

if __name__ == "__main__":
    test_hex_roundtrip()
    print("All HEX ↔ Base76 tests passed.")
