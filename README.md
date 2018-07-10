# PACO
## floating Point to fixed Point Automatic Conversion & Optimization
### Error Propagator for Fixed Point Computations

An LLVM opt pass that computes errors propagated in programs containing fixed point computations.

### Build and run

This pass requires LLVM 6.0.0 to be built and installed.
Build with:
```
$ cd COT-Error-Propagation-Project
$ mkdir build
$ cd build
$ LLVM_DIR="/path/to/llvm/install" cmake ..
$ make
```

Then, the pass can be run with
```
$ opt -load /path/to/COT-Error-Propagation-Project/build/ErrorPropagator/EPUtils/libLLVMEPUtils.so -load /path/to/COT-Error-Propagation-Project/build/ErrorPropagator/libLLVMErrorPropagator -errorprop source.ll
```

You may run the lit regression tests with
```
$ llvm-lit /path/to/COT-Error-Propagation-Project/test/ErrorPropagator
```
