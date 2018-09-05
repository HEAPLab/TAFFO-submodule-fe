# TAFFO Error Propagator for Fixed Point Computations

An LLVM opt pass that computes errors propagated in programs containing fixed point computations.

## Build and run

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

## Usage

TAFFO Error Propagator will propagate initial errors and rounding errors for all instructions whose operands meet the following requirements:
- There must be a range attached to them by means of input info metadata (cf. `Metadata.md`);
  this requirement is relaxed for instruction that do not (usually) change the range of their single operand,
  such as extend and certain conv instructions.
- An absolute error for them must be available, either because it has been propagated by this pass, or because it is attached to them as initial error in input info metadata.
  If both error sources are available, TAFFO gives precedence to the computed error.
  Whenever the initial error is used for an instruction, TAFFO emits a warning as a debug message.

Moreover, TAFFO needs type info, i.e. bit width and number of fractional bits of instructions and global variables in order to compute truncation errors for fixed point operations.
Type info, range and initial error may be attached to instructions and global variables as input info metadata.
Information for formal function parameters must be attached to their function.
Further documentation about the format of metadata and the APIs for setting and accessing them may be found in `Metadata.md`.
Note that when a range or an initial error are attached to arrays or pointers to arrays, they are considered valid and equal for each element of the array
(one may still inspect the absolute errors attached to intermediate instructions in case elements of an array are not used homogeneously).

An important caveat: TAFFO uses Alias Analysis to retrieve errors associated to the values loaded by `load` instructions.
For it to function properly, the input LLVM IR file must be in proper SSA form.
Therefore, the `-mem2reg` pass must be schedued before this pass.
