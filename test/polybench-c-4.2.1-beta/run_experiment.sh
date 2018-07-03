#!/bin/bash

if [ $# -lt 1 ]; then
  printf 'Usage: %s machine_name [n_tries]\n' $0
  printf 'machine_name  Base name for the test files:\n'
  printf '              Test output: machine_name_std.txt, machine_name_64.txt\n'
  printf '              Build log: machine_name_build.log\n'
  printf 'n_tries       Number of times the benchmarks are repeated. Default = 20\n'
  exit;
fi

TRIES=$2
if [ 'x'$TRIES = 'x' ]; then
  TRIES=20;
fi
NAME=$1

main() {
  if [ "x$NO32" = 'x' ]; then
    echo Compiling 32 bit
    ./compile_everything.sh 2> "${NAME}_build.log"
    echo Running 32 bit
    ./chkval_everything_better.py $TRIES > "${NAME}_std.txt"
    rm -r -f raw-times-32
    mv raw-times raw-times-32;
  fi
  if [ "x$NO64" = 'x' ]; then
    echo Compiling 64 bit
    ./compile_everything.sh 64bit 2>> "${NAME}_build.log"
    echo Running 64 bit
    ./chkval_everything_better.py $TRIES > "${NAME}_64.txt"
    rm -r -f raw-times-64
    mv raw-times raw-times-64;
  fi
}

trap "trap - SIGTERM && kill -- -$$" SIGINT SIGTERM
main & wait
