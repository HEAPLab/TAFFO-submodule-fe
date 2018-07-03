#!/bin/bash

if [ "x$FORMAT" = "x" ]; then
        FORMAT='\033[33m%15s\033[39m%10s%10s\033[%sm%9s%9s\033[39m%11s%13s\n'
fi
source common_chkval.sh
check "adi"
check "fdtd-2d"
check "heat-3d"
check "jacobi-1d"
check "jacobi-2d"
check "seidel-2d"
