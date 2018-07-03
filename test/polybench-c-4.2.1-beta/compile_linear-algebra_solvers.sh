#!/bin/bash

source common_compile.sh

ROOT='linear-algebra/solvers'
compile "cholesky" $D_STANDARD_DATASET    30 32 62 64
compile "durbin" $D_STANDARD_DATASET      30 32 62 64
compile "gramschmidt" $D_STANDARD_DATASET 10 32 17 64
compile "lu" $D_STANDARD_DATASET          30 32 49 64
compile "ludcmp" $D_STANDARD_DATASET      22 32 54 64
compile "trisolv" $D_STANDARD_DATASET     22 32 47 64

