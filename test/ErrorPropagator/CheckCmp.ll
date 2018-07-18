; RUN: opt -load %eputilslib -load %errorproplib -errorprop -cmpthresh 25 -S %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK: %cmp = icmp slt i32 %a, %b, !errorprop.wrongcmptol !5
; CHECK: %sub = sub nsw i32 %b, %a, !errorprop.range !8, !errorprop.abserror !9
; CHECK: %sub1 = sub nsw i32 %a, %b, !errorprop.range !10, !errorprop.abserror !9
; CHECK: %c.0 = phi i32 [ %sub, %if.then ], [ %sub1, %if.else ], !errorprop.range !11, !errorprop.abserror !9
; Function Attrs: noinline uwtable
define i32 @bar(i32 %a, i32 %b) !errorprop.argsrange !2 {
entry:
  %cmp = icmp slt i32 %a, %b
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %sub = sub nsw i32 %b, %a, !errorprop.range !8
  br label %if.end

if.else:                                          ; preds = %entry
  %sub1 = sub nsw i32 %a, %b, !errorprop.range !9
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %c.0 = phi i32 [ %sub, %if.then ], [ %sub1, %if.else ], !errorprop.range !10
  ret i32 %c.0
}

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.1 (https://git.llvm.org/git/clang.git/ 0e746072ed897a85b4f533ab050b9f506941a097) (git@github.com:llvm-mirror/llvm.git 38c1684af2387229b58d2ca8c57202ed3e60e1e3)"}
!2 = !{!3, !4}
!3 = !{!5, !6}
!5 = !{i32 -5, i32 128, i32 160}
!6 = !{double 1.000000e+00}
!4 = !{!7, !6}
!7 = !{i32 -5, i32 192, i32 224}
!8 = !{i32 -5, i32 64, i32 64}
!9 = !{i32 -5, i32 -64, i32 -64}
!10 = !{i32 -5, i32 -64, i32 64}

; CHECK: !5 = !{double 1.000000e+00}
; CHECK: !9 = !{double 2.000000e+00}
