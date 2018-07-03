# NOT TO BE EXECUTED BY ITSELF

DATADIR='./result-data'


PRINTF="./printfx.py"

TASKSET=""
which taskset > /dev/null
if [ $? -eq 0 ]; then
        TASKSET="taskset -c 0 "
fi

STACKSIZE='unlimited'
if [ $(uname -s) = "Darwin" ]; then
  STACKSIZE='65532';
fi
ulimit -s $STACKSIZE

NOERROR=0
NORUN=0
FORCEDATADIR=''

if [ "x$CHKVAL_ONLY" = 'x' ]; then
  CHKVAL_ONLY='(.*)';
fi

check() {
  if [[ ! ( "$1" =~ $CHKVAL_ONLY ) ]]; then
    return;
  fi
  
  OPT="./build/$1_out"
  NOPT="./build/$1_out_not_opt"
  
  if [[ ( $NORUN -eq 0 ) || ( "x$FORCEDATADIR" = 'x' ) ]]; then
    if [ -e "./build/$1_64" ]; then
      DATADIR='./output-data-64';
    else
      DATADIR='./output-data-32';
    fi;
  else
    DATADIR="$FORCEDATADIR";
  fi
  mkdir -p $DATADIR
  
  OPT_OUT="$DATADIR/$1_out.output.csv"
  NOPT_OUT="$DATADIR/$1_out_not_opt.output.csv"
  
  if [ $NORUN -eq 0 ]; then
    FIXT=$($TASKSET $OPT 2> $OPT_OUT)
    FLOT=$($TASKSET $NOPT 2> $NOPT_OUT);
  else
    FIXT='0'
    FLOT='0';
  fi
  
  if [ $NOERROR -eq 1 ]; then
    OFLC_OPT='0'
    OFLC_NOPT='0'
    ERROR='0'
    ABSERROR='0'
    OFLC='39';
  else
    RESDIFF=($(./resultdiff.py "$OPT_OUT" "$NOPT_OUT"))
    OFLC_OPT=${RESDIFF[0]}
    OFLC_NOPT=${RESDIFF[1]}
    if [ "$OFLC_OPT" = "$OFLC_NOPT" ]; then
      OFLC='32';
    else
      OFLC='31';
    fi
    ERROR=${RESDIFF[2]}
    ABSERROR=${RESDIFF[3]}
  fi
  
  $PRINTF "$FORMAT" $1 $FIXT $FLOT $OFLC $OFLC_OPT $OFLC_NOPT $ERROR $ABSERROR
}

if [ "x$NOHEADER" = "x" ]; then
        $PRINTF "$FORMAT" '' 'fix T' 'flo T' '39' '# ofl fix' '# ofl flo' 'avg err %' 'avg abs err'
fi

for arg; do
  case $arg in
    --noerror)
      NOERROR=1
      ;;
    --norun)
      NORUN=1
      ;;
    --64bit)
      FORCEDATADIR='./output-data-64'
      ;;
    --32bit)
      FORCEDATADIR='./output-data-32'
      ;;
    *)
      check $arg
      exit 0
  esac
done

