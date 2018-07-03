#!/bin/bash

source common_compile.sh

ROOT='linear-algebra/kernels'
compile "2mm" $D_STANDARD_DATASET       17 32 49 64
compile "3mm" $D_STANDARD_DATASET       21 32 53 64
compile "atax" $D_STANDARD_DATASET      19 32 51 64
compile "bicg" $D_STANDARD_DATASET      23 32 55 64
compile "doitgen" $D_STANDARD_DATASET   26 32 58 64
compile "mvt" $D_STANDARD_DATASET       23 32 55 64
