# TAFFO-ERROR Metadata Format

## Input Data

### Variable Range
A variable range is represented in the following way:
```
!n = !{i32 Width, i32 FracBits, double Min, double Max}
```
where:
- abs(Width) is the bit width of the fixed point format.
  Width > 0 if the format is unsigned,
  Width < 0 if the format is signed.
- FracBits is the number of bits to the right of the dot of the fixed point format.
- Min is the lower bound of the range.
- Max is the upper bound of the range.

### Instruction Range
It is a variable range attached to an Instruction, labeled as !errorprop.range.
Example:
```
%add = add nsw i32 %0, %1, !errorprop.range !5
```
And, at the end of the file,
```
!5 = !{i32 32, i32 5, double 2.0e+01, double 1.0e+2}
```
Related functions:
```
#include "EPUtils/Metadata.h"
llvm::Optional<FPInterval> retrieveRangeFromMetadata(llvm::Instruction &I);
void setRangeMetadata(llvm::Instruction &I, const FPInterval &FPI);
```
(See Doxygen comments for details.)

### Global variable name
A pair whose first element is the range and the second is the initial error.
Label: `!errorprop.globalre`.
```
@a = global i32 5, align 4, !errorprop.globalre !0
```
At the end of the file:
```
!0 = !{!1, !2}
!1 = !{i32 32, i32 5, double 2.0e+01, double 1.0e+2}
!2 = !{double 1.000000e-02}
```
Related functions:
```
#include "EPUtils/Metadata.h"
void setGlobalVariableMetadata(llvm::GlobalObject &V,
			       const FixedPointValue *Range,
			       const AffineForm<inter_t> *Error);

bool hasGlobalVariableMetadata(const llvm::GlobalObject &V);

std::pair<FPInterval, AffineForm<inter_t> >
retrieveGlobalVariableRangeError(const llvm::GlobalObject &V);
```

### Function Arguments Range and Initial Error
A list of a range/error pair for each argument in the order they appear,
attached to the function. The label is `!errorprop.argsrange`.
Example:
```
define i32 @slarti(i64 %x, i64 %y, i32 %n) #0 !errorprop.argsrange !2 {
...
```
at the end of the file:
```
!2 = !{!3, !4, !5} ; !3 = %x, !4 = %y, !5 = %n
!3 = !{!6, !7} ; !6 = range of %x, !7 = initial error of %x
!6 = !{i32 -64, i32 10, double 1.0e+00, double 5.0e+00}
!7 = !{double 1.000000e-03}
!4 = !{!8, !9}
!8 = !{i32 -64, i32 10, double -5.0e+00, double 6.0e+00}
!9 = !{double 2.000000e-03}
!5 = !{!10, !11}
!10 = !{i32 32, i32 0, double 0.0e+00, double 2.0e+01}
!11 = !{double 0.000000e+01}
```
Related functions:
```
#include "EPUtils/Metadata.h"
void setFunctionArgsMetadata(llvm::Function &,
			     const llvm::ArrayRef<std::pair<
			     const FixedPointValue *,
			     const AffineForm<inter_t> *> >);

llvm::SmallVector<std::pair<FPInterval, AffineForm<inter_t> >, 1U>
retrieveArgsRangeError(const llvm::Function &);
```

### Loop unroll count
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

### Max Recursion Count
The maximum number of recursive calls to this function allowed.
Represented as an i32 constant, labeled `!errorprop.maxrec`.
```
define i32 @foo(i32 %x, i32 %y) !errorprop.argsrange !0 !errorprop.maxrec !9 {
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

## Output data

### Computed absolute error
`double` value attached to each instruction with label `!errorprop.argsrange`.
```
%add = add nsw i32 %x, %y, !errorprop.range !4, !errorprop.abserror !5
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

### Possible wrong comparison
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
