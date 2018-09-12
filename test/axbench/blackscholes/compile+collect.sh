#!/bin/bash

# $1 stats directory

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <stats dir>"
  exit
fi

bench=blackscholes

make clean
make OX=-O3
cp stats/${bench}.fixp.mix.txt $1/${bench}_out_ic_fix.txt
cp stats/${bench}.mix.txt $1/${bench}_out_ic_float.txt
cp stats/${bench}.llvm.txt $1/${bench}_out.txt
cat stats/errorprop.log | grep 'Computed error for target' > $1/${bench}_out_errorprop.txt

