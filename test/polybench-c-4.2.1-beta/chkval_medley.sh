#!/bin/bash

if [ "x$FORMAT" = "x" ]; then
        FORMAT='\033[33m%14s\033[39m%9s%9s\033[%sm%11s%11s\033[39m%11s%13s\n'
fi
source common_chkval.sh
check "deriche"
check "floyd-warshall"
check "nussinov"
