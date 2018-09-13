#!/bin/bash

if [[ -z $FORMAT ]]; then
  FORMAT='%25s %12s %12s%5s%6s%7s%12s\n'
fi


match_time()
{
  regex='^kernel[^0-9]*([0-9.]+)'
  if [[ ( $1 =~ $regex ) ]]; then
    echo ${BASH_REMATCH[1]}
  else
    echo -1
  fi
}

match_error()
{
  regex='^[^E]*Error[^0-9]*([0-9.]+)'
  if [[ ( $1 =~ $regex ) ]]; then
    echo ${BASH_REMATCH[1]}
  else
    echo -1
  fi
}

rm -rf data/output
mkdir data/output
benchmark=inversek2j

for f in data/input/*.data
do
  filename=$(basename "$f")
  extension="${filename##*.}"
  filename="${filename%.*}"
  
  if [[ -z $NORUN ]]; then
    float=$(./bin/inversek2j.out ${f} data/output/${filename}_${benchmark}_out.data)
    mfloat=$(match_time "$float")
  
    fix=$(./bin/inversek2j.out.fixp ${f} data/output/${filename}_${benchmark}_out.data.fixp)
    mfix=$(match_time "$fix")
  else
    mfloat='0'
    mfix='0'
  fi
  
  if [[ -z $NOERROR ]]; then
    error=$(python ./scripts/qos.py data/output/${filename}_${benchmark}_out.data data/output/${filename}_${benchmark}_out.data.fixp)
    merror=$(match_error "$error")
  else
    merror='0'
  fi
  
  printf "$FORMAT" "${benchmark}_${filename}" $mfix $mfloat 0 0 '-' "$merror"
done