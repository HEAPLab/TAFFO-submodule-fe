; RUN: opt -load %eputilslib -load %errorproplib -errorprop -recur 4 -S %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK: %add = add nsw i32 %x, %y, !errorprop.range !4, !errorprop.abserror !5
; CHECK: %call = call i32 @bar(i32 %add), !errorprop.range !6, !errorprop.abserror !7

define i32 @foo(i32 %x, i32 %y) !errorprop.argsrange !0 {
entry:
  %add = add nsw i32 %x, %y, !errorprop.range !4
  %call = call i32 @bar(i32 %add), !errorprop.range !5
  ret i32 %call
}

; CHECK: %mul = mul nsw i32 %x, %x, !errorprop.range !6, !errorprop.abserror !10
; CHECK: %call = call i32 @foo(i32 %mul, i32 %x), !errorprop.range !11, !errorprop.abserror !12
; CHECK: %retval.0 = phi i32 [ %call, %if.then ], [ %x, %if.else ], !errorprop.range !6, !errorprop.abserror !12

define i32 @bar(i32 %x) !errorprop.argsrange !6 {
entry:
  %cmp = icmp slt i32 %x, 10000
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %mul = mul nsw i32 %x, %x, !errorprop.range !5
  %call = call i32 @foo(i32 %mul, i32 %x), !errorprop.range !8
  br label %return

if.else:                                          ; preds = %entry
  br label %return

return:                                           ; preds = %if.else, %if.then
  %retval.0 = phi i32 [ %call, %if.then ], [ %x, %if.else ], !errorprop.range !5
  ret i32 %retval.0
}

!0 = !{!1, !1}
!1 = !{!2, !3}
!2 = !{i32 5, i32 1, i32 10000}
!3 = !{double 1.250000e-02}
!4 = !{i32 5, i32 2, i32 20000}
!5 = !{i32 5, i32 4, i32 400000000}
!6 = !{!7}
!7 = !{!4, !3}
!8 = !{i32 5, i32 8, i32 800000000}

; CHECK: !5 = !{double 2.500000e-02}
; CHECK: !7 = !{double 0x41D8B6AB8B85A04A}
; CHECK: !10 = !{double 0x402F40147AE147AF}
; CHECK: !12 = !{double 0x41B8DC877BD0D827}
