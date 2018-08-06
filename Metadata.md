# TAFFO Metadata Format

## Generic Input Information Format

The input information associated to a variable takes the form of a metadata node formatted as follows:
 
```
!n = !{type, range, initialError}
```

Each item of the metadata tuple can be either a link to another metadata tuple or the integer zero (i1 0) in case a specific information does not apply.

### ``type`` Subnode

The ``type`` subnode, if present, specifies the target type of the variable after Taffo has completed its conversion to a different format.

```
!n = !{typeFlag, ...}
```

The following items are used to further specialize the type, depending on the value of typeFlag (a MDString).

#### ``fixp`` Type Flag

```
!n = !{!"fixp", i32 Width, i32 FracBits}
```

A fixed point type where:

- abs(Width) is the bit width of the fixed point format.
  Width > 0 if the format is unsigned,
  Width < 0 if the format is signed.
- FracBits is the number of bits to the right of the dot of the fixed point format.

### ``range`` Subnode

The ``range`` subnode, which must be present, specifies the range of values admitted for the variable.

```
!n = !{double Min, double Max}
```

- Min is the lower bound of the range.
- Max is the upper bound of the range.

### ``initialError`` Subnode

The ``initialError`` subnode, if present, specifies a priori an error value for a variable. This error is initially assumed to be correct and is used to kickstart the error propagation process.

```
!n = !{double Error}
```

### Associating Input Information to IR nodes

An input information metadata can be associated to either an Instruction, the parameters of a function, or to a GlobalObject. 

#### Input Information on Instructions and GlobalObjects

The input information is attached to the instruction using the ``taffo.info`` metadata label.

**Example:**

```
@a = global i32 5, align 4, !taffo.info !0
[...]
%add = add nsw i32 %0, %1, !taffo.info !5
```

And, at the end of the file,

```
!0 = !{!1, !2, !3}
!1 = !{!"fixp", i32 32, i32 5}
!2 = !{double 2.0e+01, double 1.0e+2}
!3 = !{double 1.000000e-02}
[...]
!5 = !{!6, !7, 0}
!6 = !{!"fixp", i32 32, i32 5}
!7 = !{double 2.0e+01, double 1.0e+2}
```

Related functions:

```cpp
#include "ErrorPropagator/EPUtils/Metadata.h"

llvm::Optional<FPInterval> retrieveRangeFromMetadata(llvm::Instruction &I);

void setInputInfoMetadata(Instruction &I, const InputInfo &IInfo);

bool hasGlobalVariableMetadata(const llvm::GlobalObject &V);

std::pair<FPInterval, AffineForm<inter_t> >
retrieveGlobalVariableRangeError(const llvm::GlobalObject &V);

void setGlobalVariableMetadata(GlobalObject &V, const InputInfo &IInfo);
```

(See Doxygen comments for details.)

#### Input Information of Function Arguments

The input information metadata for all the arguments is collected, in the same order of the arguments, as a metadata tuple, which is then tagged to the function as `!taffo.funinfo`.

Example:

```
define i32 @slarti(i64 %x, i64 %y, i32 %n) #0 !taffo.funinfo !2 {
[...]
```

at the end of the file:

```
!2 = !{!3, !4, !5} ; !3 = %x, !4 = %y, !5 = %n

!3 = !{!6, !12, !7} ; !6 = range of %x, !7 = initial error of %x
!6 = !{!"fixp", i32 -64, i32 10}
!12 = !{double 1.0e+00, double 5.0e+00}
!7 = !{double 1.000000e-03}

!4 = !{!8, !13, !9}
!8 = !{!"fixp", i32 -64, i32 10}
!13 = !{double -5.0e+00, double 6.0e+00}
!9 = !{double 2.000000e-03}

!5 = !{!10, !14, !11}
!10 = !{!"fixp", i32 32, i32 0}
!14 = !{double 0.0e+00, double 2.0e+01}
!11 = !{double 0.000000e+01}
```

Related functions:

```cpp
#include "ErrorPropagator/EPUtils/Metadata.h"

void setFunctionArgsMetadata(llvm::Function &F,
			     const ArrayRef<InputInfo> AInfo);

llvm::SmallVector<std::pair<FPInterval, AffineForm<inter_t> >, 1U>
retrieveArgsRangeError(const llvm::Function &);
```

## ErrorPropagation Specific Metadata

### Input Data

#### Loop unroll count

An i32 integer constant attached to the terminator instruction of the loop header block,
with label `!errorprop.unroll`.

```
for.body:                                         ; preds = %entry, %for.body
  ...
  br i1 %exitcond, label %for.body, label %for.end, !errorprop.unroll !8
```

At the end of the file:

```
!8 = !{i32 20}
```

for an unroll count of 20.
Related functions:

```
#include "EPUtils/Metadata.h"
void setLoopUnrollCountMetadata(llvm::Loop &L, unsigned UnrollCount);
llvm::Optional<unsigned> retrieveLoopUnrollCount(const llvm::Loop &L);
```

#### Max Recursion Count

The maximum number of recursive calls to this function allowed.
Represented as an i32 constant, labeled `!errorprop.maxrec`.

```
define i32 @foo(i32 %x, i32 %y) !taffo.info !0 !errorprop.maxrec !9 {
  ...
```

At the end of the file:

```
!9 = !{i32 15}
```

Related functions:

```
#include "EPUtils/Metadata.h"
void
setMaxRecursionCountMetadata(llvm::Function &F, unsigned MaxRecursionCount);

unsigned retrieveMaxRecursionCount(const llvm::Function &F);
```

### Output data

#### Computed absolute error

`double` value attached to each instruction with label `!errorprop.argsrange`.

```
%add = add nsw i32 %x, %y, !taffo.info !4, !errorprop.abserror !5
```

At the end of the file:

```
!5 = !{double 2.500000e-02}
```

Related functions:

```
#include "EPUtils/Metadata.h"
void setErrorMetadata(llvm::Instruction &, const AffineForm<inter_t> &);

TO BE DONE
```

#### Possible wrong comparison

If a cmp instruction may give a wrong result due to the absolute error of the operands,
this is signaled with `!errorprop.wrongcmptol`.
The absolute tolerance computed for the comparison is attached to this label.
The relative threshold above which the comparison error is signaled can be set form command line.

```
%cmp = icmp slt i32 %a, %b, !errorprop.wrongcmptol !5
```

End of file:

```
!5 = !{double 1.000000e+00}
```
