; RUN: opt -load %eputilslib -load %errorproplib -errorprop -S %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK: %cmp = icmp sgt i32 %a, %b, !errorprop.wrongcmptol !6
; CHECK: %sub = sub nsw i32 %a, %b, !errorprop.range !7, !errorprop.abserror !8
; CHECK: %sub1 = sub nsw i32 %b, %a, !errorprop.range !7, !errorprop.abserror !8
; CHECK: %retval.0 = phi i32 [ %sub, %if.then ], [ %sub1, %if.else ], !errorprop.range !7, !errorprop.abserror !8
; CHECK: ret i32 %retval.0, !errorprop.abserror !8
; CHECK: %add = add nsw i32 %a, %b, !errorprop.range !2, !errorprop.abserror !8
; CHECK: %call = call i32 @bar(i32 %add, i32 %b), !errorprop.range !7, !errorprop.abserror !3
; CHECK: %add1 = add nsw i32 %add, %call, !errorprop.range !12, !errorprop.abserror !13
; CHECK: %sub = sub nsw i32 0, %call, !errorprop.range !7, !errorprop.abserror !14
; CHECK: %mul = mul nsw i32 %add, %sub, !errorprop.range !15, !errorprop.abserror !16
; CHECK: %retval.0 = phi i32 [ %add1, %if.then ], [ %mul, %if.end ], !errorprop.range !15, !errorprop.abserror !16
; CHECK: ret i32 %retval.0, !errorprop.abserror !16

define i32 @bar(i32 %a, i32 %b) !errorprop.argsrange !7 {
entry:
  %cmp = icmp sgt i32 %a, %b
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %sub = sub nsw i32 %a, %b, !errorprop.range !10
  br label %return

if.else:                                          ; preds = %entry
  %sub1 = sub nsw i32 %b, %a, !errorprop.range !10
  br label %return

return:                                           ; preds = %if.else, %if.then
  %retval.0 = phi i32 [ %sub, %if.then ], [ %sub1, %if.else ], !errorprop.range !10
  ret i32 %retval.0
}

define i32 @foo(i32 %a, i32 %b) !errorprop.argsrange !0 {
entry:
  %add = add nsw i32 %a, %b, !errorprop.range !6
  %call = call i32 @bar(i32 %add, i32 %b), !errorprop.range !10
  %cmp = icmp sgt i32 %call, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %add1 = add nsw i32 %add, %call, !errorprop.range !11
  br label %return

if.end:                                           ; preds = %entry
  %sub = sub nsw i32 0, %call, !errorprop.range !10
  %mul = mul nsw i32 %add, %sub, !errorprop.range !12
  br label %return

return:                                           ; preds = %if.end, %if.then
  %retval.0 = phi i32 [ %add1, %if.then ], [ %mul, %if.end ], !errorprop.range !12
  ret i32 %retval.0
}

!0 = !{!1, !4}
!1 = !{!2, !3}
!2 = !{i32 -6, i32 -320, i32 320}
!3 = !{double 1.000000e-02}
!4 = !{!5, !3}
!5 = !{i32 -6, i32 -384, i32 384}
!6 = !{i32 -6, i32 -704, i32 704}
!7 = !{!8, !9}
!8 = !{!6, !3}
!9 = !{!5, !3}
!10 = !{i32 -6, i32 -1088, i32 1088}
!11 = !{i32 -6, i32 -1792, i32 1792}
!12 = !{i32 -6, i32 -11968, i32 11968}

; CHECK: !6 = !{double 1.562500e-02}
; CHECK: !8 = !{double 2.000000e-02}
; CHECK: !13 = !{double 3.000000e-02}
; CHECK: !14 = !{double 0x3F9A3D70A3D70A3E}
; CHECK: !16 = !{double 0x3FE3EA9930BE0DED}
