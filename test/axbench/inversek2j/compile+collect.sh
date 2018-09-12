#!/bin/bash

# $1 stats directory

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <stats dir>"
  exit
fi

make clean
make OX=-O3
cp stats/main.fixp.mix.txt $1/inversej2k_out_ic_fix.txt
cp stats/main.mix.txt $1/inversej2k_out_ic_float.txt
cp stats/main.llvm.txt $1/inversej2k_out.txt

