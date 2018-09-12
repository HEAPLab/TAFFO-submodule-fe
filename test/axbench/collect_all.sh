#!/bin/bash


# $1 benchmark to check
collect()
{
  pushd $1 > /dev/null
  ./compile+collect.sh "$RESULTS_DIR/stats"
  popd > /dev/null
}


main()
{
  rm -r ./raw-times
  RESULTS_DIR=$(pwd)/results
  mkdir -p "$RESULTS_DIR"
  mkdir -p "$RESULTS_DIR/stats"

  collect 'inversek2j'

  ./chkval_all.sh > "$RESULTS_DIR/error.txt"
  ./chkval_all_better.py 5 > "$RESULTS_DIR/times.txt"
  hostname > "$RESULTS_DIR/MACHINE.txt"
  mv ./raw-times "$RESULTS_DIR/"
}


trap "trap - SIGTERM && kill -- -$$" SIGINT SIGTERM
main & wait

