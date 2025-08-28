# =============================================================================
# Makefile
# Build automation for Base76 conversion utilities in C
#
# Targets:
#   make all        – Compile dec2base76 and hex2base76 from C sources
#   make clean      – Remove all compiled binaries
#
# Project Origin:
#   Source Code produced by Willtech 2023
#   C translation v1.0 — based on validated Python implementations
#   Converted & annotated with reference headers by
#   Professor. Damian A. James Williamson Grad. + Copilot
#
# Usage Examples:
#   ./dec2base76 12345          → Decimal to Base76
#   ./dec2base76 -r 3d!         → Base76 to Decimal
#   ./hex2base76 DEADBEEF       → HEX to Base76
#   ./hex2base76 -r 3d!         → Base76 to HEX
#
# Notes:
#   - Requires gcc or compatible compiler
#   - BASE76_ALPHABET constant defined in base76_alphabet.h
# =============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -O2

all: dec2base76 hex2base76

dec2base76: dec2base76.c base76_alphabet.h
	$(CC) $(CFLAGS) -o dec2base76 dec2base76.c

hex2base76: hex2base76.c base76_alphabet.h
	$(CC) $(CFLAGS) -o hex2base76 hex2base76.c

clean:
	rm -f dec2base76 hex2base76
