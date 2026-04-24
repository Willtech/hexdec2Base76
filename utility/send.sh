#!/bin/ksh
cd send
split -b 100k random.bin chunk_ && \
find . -name "chunk_*" | xargs -P 8 -I {} sh -c 'pv -N "{}" "{}" | b76pipe_stream -o "{}.b76" && rm "{}"' && \
sha256 !(*.SUM) > SHA256.SUM && sha256 -c SHA256.SUM && \
echo "Check Okay!" || echo "Check Failed!"
