#!/bin/bash

source common_compile.sh

ROOT='medley'
compile "deriche" $D_STANDARD_DATASET         29 32 61 64
compile "floyd-warshall" $D_STANDARD_DATASET  20 32 52 64
compile "nussinov" $D_STANDARD_DATASET        23 32 31 64
