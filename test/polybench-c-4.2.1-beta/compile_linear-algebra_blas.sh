#!/bin/bash

source common_compile.sh

ROOT='linear-algebra/blas'
compile "gemm" $D_STANDARD_DATASET      24 32 56 64
compile "gemver" $D_STANDARD_DATASET    18 32 50 64
compile "gesummv" $D_STANDARD_DATASET   23 32 55 64
compile "symm" $D_STANDARD_DATASET      26 32 58 64
compile "syr2k" $D_STANDARD_DATASET     23 32 55 64
compile "syrk" $D_STANDARD_DATASET      24 32 56 64
compile "trmm" $D_STANDARD_DATASET      23 32 56 64
