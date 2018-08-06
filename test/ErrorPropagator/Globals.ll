; RUN: opt -load %eputilslib -load %errorproplib -errorprop -S %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@a = global i32 5, align 4, !taffo.info !0
@b = global i32 10, align 4, !taffo.info !4

; CHECK: %0 = load i32, i32* @a, align 4, !errorprop.abserror !2
; CHECK: %add = add nsw i32 %c, %0, !errorprop.range !9, !errorprop.abserror !5
; CHECK: store i32 %add, i32* @a, align 4, !errorprop.abserror !5
; CHECK: %1 = load i32, i32* @b, align 4, !errorprop.abserror !5
; CHECK: %mul = mul nsw i32 %c, %1, !errorprop.range !10, !errorprop.abserror !11
; CHECK: store i32 %mul, i32* @b, align 4, !errorprop.abserror !11
; CHECK: %2 = load i32, i32* @a, align 4, !errorprop.abserror !5
; CHECK: %3 = load i32, i32* @b, align 4, !errorprop.abserror !11
; CHECK: %add1 = add nsw i32 %2, %3, !errorprop.range !12, !errorprop.abserror !13
; CHECK: ret i32 %add1, !errorprop.abserror !13

; Function Attrs: noinline nounwind uwtable
define i32 @foo(i32 %c) !taffo.funinfo !7 {
entry:
  %0 = load i32, i32* @a, align 4
  %add = add nsw i32 %c, %0, !taffo.info !11
  store i32 %add, i32* @a, align 4
  %1 = load i32, i32* @b, align 4
  %mul = mul nsw i32 %c, %1, !taffo.info !13
  store i32 %mul, i32* @b, align 4
  %2 = load i32, i32* @a, align 4
  %3 = load i32, i32* @b, align 4
  %add1 = add nsw i32 %2, %3, !taffo.info !15
  ret i32 %add1
}

!0 = !{!1, !2, !3}
!1 = !{!"fixp", i32 32, i32 5}
!2 = !{double 4.000000e+00, double 6.000000e+00}
!3 = !{double 1.000000e-02}
!4 = !{!1, !5, !6}
!5 = !{double 9.000000e+00, double 1.100000e+01}
!6 = !{double 2.000000e-02}
!7 = !{!8}
!8 = !{!1, !9, !10}
!9 = !{double 2.000000e+00, double 3.000000e+00}
!10 = !{double 1.000000e-02}
!11 = !{!1, !12, i1 0}
!12 = !{double 6.000000e+00, double 9.000000e+00}
!13 = !{!1, !14, i1 0}
!14 = !{double 5.760000e+02, double 1.056000e+03}
!15 = !{!1, !16, i1 0}
!16 = !{double 5.820000e+02, double 1.065000e+03}

; CHECK: !11 = !{double 1.702000e-01}
; CHECK: !13 = !{double 1.902000e-01}
