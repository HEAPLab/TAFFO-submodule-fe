# AxBench TAFFO Benchmarks

Build and run with:
```
$ export INITLIB='/path/to/taffo-wrapper/Initializer/build/TaffoInitializer/TaffoInitializer.so'
$ export PASSLIB='/path/to/taffo-wrapper/Conversion/build/LLVMFloatToFixed/LLVMFloatToFixed.so'
$ export ERRORLIB='/path/to/taffo-wrapper/ErrorAnalysis/build/ErrorPropagator/LLVMErrorPropagator.so'
$ cd testfolder
$ make all
```
Note that the `clang` and `opt` binaries of the LLVM install against which the pass libraries have been compiled must be in the current `$PATH`.
