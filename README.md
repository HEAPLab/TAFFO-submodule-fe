# PACO
## floating Point to fixed Point Automatic Conversion & Optimization
### Error Propagator for Fixed Point Computations

An LLVM opt pass that computes errors propagated in programs containing fixed point computations.

### Build and run

This pass requires LLVM 6.0.1 to be built and installed.
Build with:
```
$ cd paco-error
$ mkdir build
$ cd build
$ LLVM_DIR="/path/to/llvm/install" cmake ..
$ make
```

Then, the pass can be run with
```
$ opt -load /path/to/paco-error/build/ErrorPropagator/EPUtils/libLLVMEPUtils.so -load /path/to/paco-error/build/ErrorPropagator/libLLVMErrorPropagator -errorprop source.ll
```

You may run the lit regression tests with
```
$ llvm-lit /path/to/paco-error/test/ErrorPropagator
```
