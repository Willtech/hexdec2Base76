#!/bin/ksh
echo "Stage 1"
cat ../python/message1
echo "Stage 2"
./b76pipe -i ../python/message1
echo "Stage 3"
./b76pipe -v -i ../python/message1
echo "Stage 4"
./b76pipe -i ../python/message1 -o convert
echo "Stage 5"
./b76pipe -r -i convert
echo "Stage 6"
./b76pipe -r -v -i convert
