; RUN: opt -load %eputilslib -load %errorproplib -errorprop -S %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK: %add = add nsw i32 %a.addr.02, %a.addr.02, !errorprop.range !6, !errorprop.abserror !7
; CHECK: %a.addr.0.lcssa = phi i32 [ %add, %for.body ], !errorprop.abserror !8
; CHECK: %mul = mul nsw i32 %a.addr.0.lcssa, %a.addr.0.lcssa, !errorprop.range !9, !errorprop.abserror !10
; Function Attrs: noinline uwtable
define i32 @foo(i32 %a) #0 !errorprop.argsrange !2 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.body
  %a.addr.02 = phi i32 [ %a, %entry ], [ %add, %for.body ]
  %i.01 = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  %add = add nsw i32 %a.addr.02, %a.addr.02, !errorprop.range !6
  %inc = add nuw nsw i32 %i.01, 1
  %exitcond = icmp ne i32 %inc, 10
  br i1 %exitcond, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  %a.addr.0.lcssa = phi i32 [ %add, %for.body ]
  %mul = mul nsw i32 %a.addr.0.lcssa, %a.addr.0.lcssa, !errorprop.range !7
  ret i32 %mul
}

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.1 (https://git.llvm.org/git/clang.git/ 0e746072ed897a85b4f533ab050b9f506941a097) (git@github.com:llvm-mirror/llvm.git 7883f391cb5539d062f0d6d9b3aa05b159b18450)"}
!2 = !{!3}
!3 = !{!4, !5}
!4 = !{i32 -4, i32 80, i32 96}
!5 = !{double 1.250000e-02}
!6 = !{i32 -4, i32 800, i32 960}
!7 = !{i32 -4, i32 40000, i32 57600}
; CHECK: !7 = !{double 2.500000e-02}
; CHECK: !8 = !{double 1.280000e+01}
; CHECK: !10 = !{double 0x409A8F5C28F5C290}
