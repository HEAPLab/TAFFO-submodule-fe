#!/bin/bash -x

ROOT=$(dirname "$0")

if [ $# -lt 1 ]; then
  echo "usage: $0 <filename.c>";
  echo 'compiles a binary but by converting floats to fixed point first';
  echo 'output to filename (without the extension) and to various _tmp* files';
  echo 'any existing files will be overwritten so be careful';
  exit;
fi

if [ -z ${LLVM_DIR+x} ]; then
  echo 'please set $LLVM_DIR to a prefix where LLVM 4.0 can be found';
  exit;
fi

if [ 'x'"$LOG_ONLY" != 'x' ]; then
  exit;
fi

OUTDIR='./build'
mkdir -p $OUTDIR

SOEXT="so"
if [ $(uname -s) = "Darwin" ]; then
  SOEXT="dylib";
fi

CLANG=$LLVM_DIR/bin/clang
OPT=$LLVM_DIR/bin/opt
LLC=$LLVM_DIR/bin/llc
PASSLIB="$ROOT/../../build/LLVMFloatToFixed/Debug/LLVMFloatToFixed.$SOEXT"
INSTCNT="$ROOT/../../build/tool/Debug/istr_type"
if [ ! -e "$PASSLIB" ]; then
  PASSLIB="$ROOT/../../build/LLVMFloatToFixed/LLVMFloatToFixed.$SOEXT";
  INSTCNT="$ROOT/../../build/tool/istr_type"
fi
OUTNAME=$(echo "$5" | sed -E 's/\.[^\.]$//')

ISDEBUG=$("$OPT" --version | grep DEBUG | wc -l)
DEBUGONLYFLAG="-debug-only=flttofix"
if [ $ISDEBUG != '1' ]; then
  DEBUGONLYFLAG='';
fi

$CLANG -S -emit-llvm "$1" -o "$OUTDIR/_tmp0.$5.ll" $3 $4

if [ 'x'$COLLECT_STATS_DIR != 'x' ]; then
  $OPT -load="$PASSLIB" -S -flttofix -dce -stats "$OUTDIR/_tmp0.$5.ll" -o "$OUTDIR/_tmp1.$5.ll" $7 2> "$COLLECT_STATS_DIR/${OUTNAME}.txt"
  echo $(wc -l < "$1") 'LOC' >> "$COLLECT_STATS_DIR/${OUTNAME}.txt"
  $INSTCNT "$OUTDIR/_tmp0.$5.ll" > "$COLLECT_STATS_DIR/${OUTNAME}_ic_float.txt"
  $INSTCNT "$OUTDIR/_tmp1.$5.ll" > "$COLLECT_STATS_DIR/${OUTNAME}_ic_fix.txt"
  exit 0
fi

$OPT -load="$PASSLIB" -pass-remarks=flttofix -S -flttofix -dce $DEBUGONLYFLAG "$OUTDIR/_tmp0.$5.ll" -o "$OUTDIR/_tmp1.$5.ll" $7
$CLANG -S -o "$OUTDIR/_tmp2.$5.s" "$OUTDIR/_tmp1.$5.ll" $2 $3
$CLANG -o "$OUTDIR/$OUTNAME" "$OUTDIR/_tmp2.$5.s" $2 $3 $6

if [ 'x'$DONT_RECOMPILE_FLOAT != 'x' ]; then
  exit 0
fi
$CLANG -S -o "$OUTDIR/_tmp2_not_opt.$5.s" "$OUTDIR/_tmp0.$5.ll" $2 $3
$CLANG -o "$OUTDIR/$OUTNAME""_not_opt" "$OUTDIR/_tmp2_not_opt.$5.s" $2 $3 $6
