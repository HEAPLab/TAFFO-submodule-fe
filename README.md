# TAFFO Error Propagator for Fixed Point Computations

An LLVM opt pass that computes errors propagated in programs containing fixed point computations.

### Build and run

This pass requires LLVM 6.0.1 to be built and installed.
Build with:
```
$ cd taffo-error
$ mkdir build
$ cd build
$ LLVM_DIR="/path/to/llvm/install" cmake ..
$ make
```

Then, the pass can be run with
```
$ opt -load /path/to/taffo-error/build/ErrorPropagator/libLLVMErrorPropagator.so -errorprop source.ll
```

You may run the lit regression tests with
```
$ llvm-lit /path/to/taffo-error/test/ErrorPropagator
```
