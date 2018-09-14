#!/bin/bash

export FORMAT='%40s %12s %12s%11s%11s%14s%20s\n'


# $1 benchmark to check
check()
{
  pushd $1 > /dev/null
  ./run2.sh
  popd > /dev/null
}


printf "$FORMAT" '' 'fix T' 'flo T' '# ofl fix' '# ofl flo' 'avg err %' 'avg abs err'

for arg; do
  case $arg in
    --noerror)
      export NOERROR=1
      ;;
    --norun)
      export NORUN=1
      ;;
    *)
      check $arg
      exit 0
  esac
done

check 'blackscholes'
check 'fft'
check 'inversek2j'
check 'jmeint'
check 'kmeans'
check 'sobel'



