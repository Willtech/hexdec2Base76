#!/bin/ksh

# demo.sh — automated send/transfer/receive pipeline
# Runs from ./utility/ and must not exit on error.

set +e   # ensure the script NEVER exits on error

echo "[1/7] Running cleanup.sh"
./cleanup.sh || echo "cleanup.sh failed (continuing)"

echo "[2/7] Detecting filename in send/ (excluding SHA256.SUM)"
FILENAME=$(ls send 2>/dev/null | grep -v '^SHA256.SUM$' | head -n 1)

if [ -z "$FILENAME" ]; then
    echo "ERROR: No file found in send/ (other than SHA256.SUM). Continuing anyway."
else
    echo "Detected filename: $FILENAME"
fi

echo "[3/7] Patching send.sh and receive.sh (random.bin → $FILENAME)"
if [ -n "$FILENAME" ]; then
    sed -i "s/random\.bin/$FILENAME/g" send.sh      || echo "sed failed on send.sh"
    sed -i "s/random\.bin/$FILENAME/g" receive.sh   || echo "sed failed on receive.sh"
else
    echo "Skipping sed replacement because no filename was detected."
fi

echo "[4/7] Running send.sh"
./send.sh || echo "send.sh failed (continuing)"

echo "[5/7] Running transfer.sh"
./transfer.sh || echo "transfer.sh failed (continuing)"

echo "[6/7] Running receive.sh"
./receive.sh || echo "receive.sh failed (continuing)"

echo "[7/7] Restoring original send.sh and receive.sh from bak/"
cp bak/send.sh send.sh       || echo "Failed to restore send.sh"
cp bak/receive.sh receive.sh || echo "Failed to restore receive.sh"

echo "demo.sh complete."
