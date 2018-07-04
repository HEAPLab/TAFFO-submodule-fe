; RUN: opt -load %eputilslib -load %errorproplib -errorprop -S %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK: %0 = sub i32 0, %b, !errorprop.range !9, !errorprop.abserror !10
; CHECK: %a.addr.0.p = select i1 %cmp, i32 %b, i32 %0, !errorprop.range !11, !errorprop.abserror !10
; CHECK: %a.addr.0 = add i32 %a.addr.0.p, %a, !errorprop.range !12, !errorprop.abserror !13
; CHECK: %mul = mul nsw i32 %a.addr.0, %b, !errorprop.range !14, !errorprop.abserror !15
; CHECK: %div = sdiv i32 %mul, %b, !errorprop.range !16, !errorprop.abserror !17
; Function Attrs: norecurse nounwind readnone uwtable
define i32 @foo(i32 %a, i32 %b) local_unnamed_addr #0 !errorprop.argsrange !3 {
entry:
  %cmp = icmp slt i32 %a, %b
  %0 = sub i32 0, %b, !errorprop.range !10
  %a.addr.0.p = select i1 %cmp, i32 %b, i32 %0, !errorprop.range !11
  %a.addr.0 = add i32 %a.addr.0.p, %a, !errorprop.range !12
  %mul = mul nsw i32 %a.addr.0, %b, !errorprop.range !13
  %div = sdiv i32 %mul, %b, !errorprop.range !14
  ret i32 %div
}

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.1 (https://git.llvm.org/git/clang.git/ 0e746072ed897a85b4f533ab050b9f506941a097) (git@github.com:llvm-mirror/llvm.git ce53c20d527634abbccce7caf92891517ba0ab30)"}
!3 = !{!4, !7}
!4 = !{!5, !6}
!5 = !{i32 -3, i32 8, i32 12}
!6 = !{double 1.000000e-02}
!7 = !{!8, !9}
!8 = !{i32 -3, i32 10, i32 14}
!9 = !{double 2.000000e-03}
!10 = !{i32 -3, i32 -14, i32 -10}
!11 = !{i32 -3, i32 -10, i32 14}
!12 = !{i32 -3, i32 -6, i32 26}
!13 = !{i32 -3, i32 -84, i32 364}
!14 = !{i32 -3, i32 -8, i32 26}
; CHECK: !15 = !{double 2.465240e-01}
; CHECK: !16 = !{i32 -3, i32 -8, i32 26}
; CHECK: !17 = !{double 0x40104FC903E8FCE9}