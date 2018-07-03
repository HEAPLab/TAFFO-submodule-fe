#!/bin/bash

./compile_everything.sh --stats-only 2> /dev/null > /dev/null
benchlist=($(./compile_everything.sh --dump-bench-list | tr '\n' ' '))

for bench in ${benchlist[@]}; do
  fn='stats/'${bench}'_out.txt'
  loc=$(grep 'LOC' "$fn" | grep -o -E '[0-9]+')
  anno=$(grep 'valid annotations' "$fn" | grep -o -E '[0-9]+')
  locperanno=$(echo "scale=1;${anno}*100/${loc}" | bc)
  echo $bench $loc $anno $locperanno
done;


