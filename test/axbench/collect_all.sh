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
  if [[ -z $1 ]]; then
    RESULTS_DIR="./results"
  else
    RESULTS_DIR="$1"
  fi
  
  if [[ -e "$RESULTS_DIR" ]]; then
    echo 'results dir already exists; archive or remove first please'
    return;
  fi

  rm -r ./raw-times
  mkdir -p "$RESULTS_DIR"
  mkdir -p "$RESULTS_DIR/stats"
  
  RESULTS_DIR=$(cd ${RESULTS_DIR} 2> /dev/null && pwd -P)

  collect 'blackscholes'
  collect 'fft'
  collect 'inversek2j'
  collect 'jmeint'
  collect 'kmeans'
  collect 'sobel'

  ./chkval_all.sh > "$RESULTS_DIR/error.txt"
  ./chkval_all_better.py 5 > "$RESULTS_DIR/times.txt"
  hostname > "$RESULTS_DIR/MACHINE.txt"
  mv ./raw-times "$RESULTS_DIR/"
}


trap "trap - SIGTERM && kill -- -$$" SIGINT SIGTERM
main "$1" & wait

