#!/bin/bash

source common_compile.sh

ROOT='datamining'
compile "correlation" $D_STANDARD_DATASET 22 32 54 64
compile "covariance" $D_STANDARD_DATASET  10 32 42 64
