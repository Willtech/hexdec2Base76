#!/bin/ksh
#
# Comprehensive test suite for b76pipe_stream
# Validates:
#   - stdin input
#   - -i file input
#   - stdout output
#   - -o file output
#   - verbose mode (-v)
#   - reverse mode (-r)
#   - custom alphabet (--alpha)
#   - byte-for-byte reversibility
#
# Exits immediately on any failure.
#

set -euo pipefail

BIN="../b76pipe_stream"
MSG="../../python/message1"
ALPHA="./alpha_test.txt"

# Temporary files
OUT1="./out_stage2.txt"
OUT2="./out_stage2_i.txt"
OUT3="./out_stage2_o.txt"
REV1="./rev1.txt"
REV2="./rev2.txt"
REV3="./rev3.txt"
REV4="./rev4.txt"
STAGE1="./stage1.txt"
STAGE1B="./stage1b.txt"

echo "=============================================="
echo " Base76 Pipeline Test Suite"
echo "=============================================="

###############################################################################
# 0. Pre-checks
###############################################################################

if [ ! -x "$BIN" ]; then
    echo "ERROR: $BIN not found or not executable"
    exit 1
fi

if [ ! -f "$MSG" ]; then
    echo "ERROR: Test message file $MSG not found"
    exit 1
fi

###############################################################################
# 1. Forward encode via stdin
###############################################################################
echo "[1] Forward encode via stdin → stdout"

cat "$MSG" | $BIN > "$OUT1"

echo "OK"

###############################################################################
# 2. Forward encode via -i file
###############################################################################
echo "[2] Forward encode via -i file → stdout"

$BIN -i "$MSG" > "$OUT2"

echo "OK"

###############################################################################
# 3. Forward encode via -i file and -o file
###############################################################################
echo "[3] Forward encode via -i file → -o file"

$BIN -i "$MSG" -o "$OUT3"

echo "OK"

###############################################################################
# 4. Validate Stage-2 outputs match across all methods
###############################################################################
echo "[4] Validate Stage-2 outputs match"

# Strip trailing newline from stdout outputs
tr -d '\n' < "$OUT1" > "$OUT1.nostrip"
tr -d '\n' < "$OUT2" > "$OUT2.nostrip"

cmp "$OUT1.nostrip" "$OUT3"
cmp "$OUT2.nostrip" "$OUT3"

echo "OK"

###############################################################################
# 5. Verbose mode: Stage-1 to stderr, Stage-2 to stdout
###############################################################################
echo "[5] Verbose mode test (-v)"

$BIN -v -i "$MSG" > "$OUT1" 2> "$STAGE1"

# Re-run without -v to compare Stage-2
$BIN -i "$MSG" > "$OUT2"

cmp "$OUT1" "$OUT2"

echo "OK"

###############################################################################
# 6. Reverse decode (stdin)
###############################################################################
echo "[6] Reverse decode via stdin"

cat "$OUT1" | $BIN -r > "$REV1"

cmp "$REV1" "$MSG"

echo "OK"

###############################################################################
# 7. Reverse decode via -i file
###############################################################################
echo "[7] Reverse decode via -i file"

$BIN -r -i "$OUT1" > "$REV2"

cmp "$REV2" "$MSG"

echo "OK"

###############################################################################
# 8. Reverse decode via -i and -o
###############################################################################
echo "[8] Reverse decode via -i file → -o file"

$BIN -r -i "$OUT1" -o "$REV3"

cmp "$REV3" "$MSG"

echo "OK"

###############################################################################
# 9. Reverse decode with verbose (-r -v)
###############################################################################
echo "[9] Reverse decode with verbose (-r -v)"

$BIN -r -v -i "$OUT1" > "$REV4" 2> "$STAGE1B"

cmp "$REV4" "$MSG"

echo "OK"

###############################################################################
# 10. Custom alphabet test (--alpha)
###############################################################################
echo "[10] Custom alphabet test (--alpha)"

# Build a simple alphabet file with 76 unique chars
cat > "$ALPHA" <<EOF
0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()_+[]?
EOF

# Encode with custom alphabet
$BIN --alpha "$ALPHA" -i "$MSG" > "$OUT1"

# Decode with same alphabet
$BIN --alpha "$ALPHA" -r -i "$OUT1" > "$REV1"

cmp "$REV1" "$MSG"

echo "OK"

###############################################################################
# 11. Built-in selftest (--selftest)
###############################################################################
echo "[11] Built-in selftest (--selftest)"

SELFTEST_OUTPUT=$($BIN --selftest)

if [ "$SELFTEST_OUTPUT" != "b76pipe_stream self-test Okay." ]; then
    echo "ERROR: Selftest did not return expected success message"
    echo "Got: $SELFTEST_OUTPUT"
    exit 1
fi

# Check that the two files were created
if [ ! -f ".b76pipe_stream.selftest.i" ]; then
    echo "ERROR: .b76pipe_stream.selftest.i not created"
    exit 1
fi

if [ ! -f ".b76pipe_stream.selftest.o" ]; then
    echo "ERROR: .b76pipe_stream.selftest.o not created"
    exit 1
fi

echo "OK"

###############################################################################
# 12. Selftest with custom string (--selftest \"string\")
###############################################################################
echo "[12] Selftest with custom string"

SELFTEST_OUTPUT=$($BIN --selftest "HelloWorld123")

if [ "$SELFTEST_OUTPUT" != "b76pipe_stream self-test Okay." ]; then
    echo "ERROR: Selftest with custom string failed"
    exit 1
fi

# Validate .i contains the custom string
if ! grep -q "HelloWorld123" .b76pipe_stream.selftest.i; then
    echo "ERROR: .b76pipe_stream.selftest.i does not contain custom string"
    exit 1
fi

echo "OK"

###############################################################################
# 13. Selftest with custom file (--selftest -i file)
###############################################################################
echo "[13] Selftest with custom file"

echo "FILETEST_ABC_123" > ./selftest_input.txt

SELFTEST_OUTPUT=$($BIN --selftest -i ./selftest_input.txt)

if [ "$SELFTEST_OUTPUT" != "b76pipe_stream self-test Okay." ]; then
    echo "ERROR: Selftest with custom file failed"
    exit 1
fi

# Validate .i contains the file contents
if ! grep -q "FILETEST_ABC_123" .b76pipe_stream.selftest.i; then
    echo "ERROR: .b76pipe_stream.selftest.i does not contain file contents"
    exit 1
fi

echo "OK"

###############################################################################
# 14. Selftest error case: string + file → error
###############################################################################
echo "[14] Selftest error case (string + file)"

set +e
$BIN --selftest "ABC" -i ./selftest_input.txt > /dev/null 2>&1
RET=$?
set -e

if [ $RET -eq 0 ]; then
    echo "ERROR: Selftest should fail when both string and file are provided"
    exit 1
fi

echo "OK"

###############################################################################
# 15. Selftest combined with normal operation (--selftest -i -o)
###############################################################################
echo "[15] Selftest combined with normal operation"

$BIN --selftest -i "$MSG" -o "./combined_out.txt"

# Validate output file exists
if [ ! -f "./combined_out.txt" ]; then
    echo "ERROR: combined_out.txt not created"
    exit 1
fi

# Validate reversibility
$BIN -r -i "./combined_out.txt" > "./combined_rev.txt"

cmp "./combined_rev.txt" "$MSG"

echo "OK"

###############################################################################
# 16. Summary
###############################################################################
echo "=============================================="
echo " All tests PASSED successfully."
echo "=============================================="
