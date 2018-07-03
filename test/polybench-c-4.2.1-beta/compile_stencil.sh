#!/bin/bash

source common_compile.sh

ROOT='stencils'
compile "adi" $D_STANDARD_DATASET             21 32 53 64
compile "fdtd-2d" $D_STANDARD_DATASET         22 32 54 64
compile "heat-3d" $D_STANDARD_DATASET         26 32 58 64
compile "jacobi-1d" $D_STANDARD_DATASET       29 32 61 64
compile "jacobi-2d" $D_STANDARD_DATASET       20 32 52 64
compile "seidel-2d" $D_STANDARD_DATASET       19 32 51 64
