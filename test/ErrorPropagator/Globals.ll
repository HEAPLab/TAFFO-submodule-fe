; RUN: opt -load %eputilslib -load %errorproplib -errorprop -S %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@a = global i32 5, align 4, !errorprop.globalre !0
@b = global i32 10, align 4, !errorprop.globalre !3

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
define i32 @foo(i32 %c) !errorprop.argsrange !6 {
entry:
  %0 = load i32, i32* @a, align 4
  %add = add nsw i32 %c, %0, !errorprop.range !10
  store i32 %add, i32* @a, align 4
  %1 = load i32, i32* @b, align 4
  %mul = mul nsw i32 %c, %1, !errorprop.range !11
  store i32 %mul, i32* @b, align 4
  %2 = load i32, i32* @a, align 4
  %3 = load i32, i32* @b, align 4
  %add1 = add nsw i32 %2, %3, !errorprop.range !12
  ret i32 %add1
}

!0 = !{!1, !2}
!1 = !{i32 5, i32 128, i32 192}
!2 = !{double 1.000000e-02}
!3 = !{!4, !5}
!4 = !{i32 5, i32 288, i32 352}
!5 = !{double 2.000000e-02}
!6 = !{!7}
!7 = !{!8, !9}
!8 = !{i32 5, i32 64, i32 96}
!9 = !{double 1.000000e-02}
!10 = !{i32 5, i32 192, i32 288}
!11 = !{i32 5, i32 18432, i32 33792}
!12 = !{i32 5, i32 18624, i32 34080}

; CHECK: !11 = !{double 0x3FAED288CE703AFC}
; CHECK: !13 = !{double 0x3FB487FCB923A29D}
