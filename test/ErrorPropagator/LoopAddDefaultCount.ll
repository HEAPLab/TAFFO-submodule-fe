; RUN: opt -load %eputilslib -load %errorproplib -errorprop -dunroll 5 -S %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK: %a.addr.03 = phi i32 [ %a, %for.body.lr.ph ], [ %add, %for.body ], !errorprop.abserror !5
; CHECK: %add = add nsw i32 %a.addr.03, %a.addr.03, !errorprop.range !6, !errorprop.abserror !7
; CHECK: %split = phi i32 [ %add, %for.body ], !errorprop.range !6, !errorprop.abserror !8
; CHECK: %a.addr.0.lcssa = phi i32 [ %split, %for.cond.for.end_crit_edge ], [ %a, %entry ], !errorprop.range !6, !errorprop.abserror !8
; CHECK: %mul = mul nsw i32 %a.addr.0.lcssa, %a.addr.0.lcssa, !errorprop.range !9, !errorprop.abserror !10
; CHECK: ret i32 %mul, !errorprop.abserror !10

; Function Attrs: noinline uwtable
define i32 @foo(i32 %a, i32 %b) #0 !errorprop.argsrange !2 {
entry:
  %cmp1 = icmp slt i32 0, %b
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %a.addr.03 = phi i32 [ %a, %for.body.lr.ph ], [ %add, %for.body ]
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %add = add nsw i32 %a.addr.03, %a.addr.03, !errorprop.range !6
  %inc = add nuw nsw i32 %i.02, 1
  %exitcond = icmp ne i32 %inc, %b
  br i1 %exitcond, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  %split = phi i32 [ %add, %for.body ], !errorprop.range !6
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  %a.addr.0.lcssa = phi i32 [ %split, %for.cond.for.end_crit_edge ], [ %a, %entry ], !errorprop.range !6
  %mul = mul nsw i32 %a.addr.0.lcssa, %a.addr.0.lcssa, !errorprop.range !7
  ret i32 %mul
}

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.1 (https://git.llvm.org/git/clang.git/ 0e746072ed897a85b4f533ab050b9f506941a097) (git@github.com:llvm-mirror/llvm.git f63e4a17fa8545617336f7ebf5a418fdeee4530b)"}
!2 = !{!3}
!3 = !{!4, !5}
!4 = !{i32 -4, i32 80, i32 96}
!5 = !{double 1.250000e-02}
!6 = !{i32 -4, i32 400, i32 576}
!7 = !{i32 -4, i32 160000, i32 331776}

; CHECK: !7 = !{double 2.500000e-02}
; CHECK: !8 = !{double 4.000000e-01}
; CHECK: !10 = !{double 2.896000e+01}

