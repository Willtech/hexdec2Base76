#!/bin/ksh
# Exit immediately if any command returns a non-zero status
set -e

cd receive

echo "==> Verifying encoded chunks..."
# -q makes sha256 silent unless there is a failure
grep "chunk_.*\.b76" SHA256.SUM | sha256 -cq

echo "==> Extracting chunks (Parallel)..."
# Using -q on pv hides the progress bar if you want total silence, 
# or keep it as is for the visual stack.
find . -name "chunk_*.b76" | xargs -P 8 -I {} sh -c 'pv -N "$1" "$1" | b76pipe_stream -r -o "${1%.b76}" && rm "$1"' -- {}

echo "==> Reassembling random.bin..."
cat chunk_* > random.bin

echo "==> Final integrity check..."
grep "random.bin" SHA256.SUM | sha256 -c

echo "Success: random.bin is ready."

##REL>>>
# https://share.google/aimode/cgzNbejrTsVc4Onym
