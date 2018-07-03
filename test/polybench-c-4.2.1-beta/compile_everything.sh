#!/bin/bash

source common_compile.sh

NOTCONCURRENT=''
if [[ $TBLDUMP -eq 1 || $LISTDUMP -eq 1 ]]; then
  NOTCONCURRENT='wait';
fi

trap "trap - SIGTERM && kill -- -$$" SIGINT SIGTERM

./compile_datamining.sh "$@" &
$NOTCONCURRENT
./compile_linear-algebra_blas.sh "$@" &
$NOTCONCURRENT
./compile_linear-algebra_kernels.sh "$@" &
$NOTCONCURRENT
./compile_linear-algebra_solvers.sh "$@" &
$NOTCONCURRENT
./compile_medley.sh "$@" &
$NOTCONCURRENT
./compile_stencil.sh "$@" &
$NOTCONCURRENT
wait


