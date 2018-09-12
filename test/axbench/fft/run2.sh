#!/bin/bash

if [[ -z $FORMAT ]]; then
  FORMAT='%40s %12s %12s%5s%6s%7s%12s\n'
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
mkdir -p data/output
benchmark=fft

input=( 2048 8192 65536 4194304)

for f in "${input[@]}"
do
  if [[ -z $NORUN ]]; then
    float=$(./bin/${benchmark}.out ${f} data/output/${f}_${benchmark}_out.data)
    mfloat=$(match_time "$float")
  
    fix=$(./bin/${benchmark}.out.fixp ${f} data/output/${f}_${benchmark}_out.data.fixp)
    mfix=$(match_time "$fix")
  else
    mfloat='0'
    mfix='0'
  fi
  
  if [[ -z $NOERROR ]]; then
    error=$(python ./scripts/qos.py data/output/${f}_${benchmark}_out.data data/output/${f}_${benchmark}_out.data.fixp)
    merror=$(match_error "$error")
  else
    merror='0'
  fi
  
  printf "$FORMAT" "${benchmark}_${f}" $mfix $mfloat 0 0 '-' "$merror"
done
