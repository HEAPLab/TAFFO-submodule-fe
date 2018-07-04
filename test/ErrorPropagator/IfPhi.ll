; RUN: opt -load %eputilslib -load %errorproplib -errorprop -S %s | FileCheck %s
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK: %add = add nsw i32 %a, %b, !errorprop.range !9, !errorprop.abserror !10
; CHECK: %sub = sub nsw i32 %a, %b, !errorprop.range !11, !errorprop.abserror !10
; CHECK: %a.addr.0 = phi i32 [ %add, %if.then ], [ %sub, %if.else ], !errorprop.range !12, !errorprop.abserror !10
; CHECK: %mul = mul nsw i32 %a.addr.0, %b, !errorprop.range !13, !errorprop.abserror !14
; CHECK: %div = sdiv i32 %b, %mul, !errorprop.range !15, !errorprop.abserror !16
; Function Attrs: noinline nounwind uwtable
define i32 @foo(i32 %a, i32 %b) #0 !errorprop.argsrange !3 {
entry:
  %cmp = icmp slt i32 %a, %b
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %add = add nsw i32 %a, %b, !errorprop.range !10
  br label %if.end

if.else:                                          ; preds = %entry
  %sub = sub nsw i32 %a, %b, !errorprop.range !11
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %a.addr.0 = phi i32 [ %add, %if.then ], [ %sub, %if.else ], !errorprop.range !12
  %mul = mul nsw i32 %a.addr.0, %b, !errorprop.range !13
  %div = sdiv i32 %b, %mul, !errorprop.range !14
  ret i32 %div
}

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.1 (https://git.llvm.org/git/clang.git/ 0e746072ed897a85b4f533ab050b9f506941a097) (git@github.com:llvm-mirror/llvm.git ce53c20d527634abbccce7caf92891517ba0ab30)"}
!3 = !{!4, !7}
!4 = !{!5, !6}
!5 = !{i32 -3, i32 8, i32 12}
!6 = !{double 1.250000e-01}
!7 = !{!8, !9}
!8 = !{i32 -3, i32 10, i32 14}
!9 = !{double 2.000000e-03}
!10 = !{i32 -3, i32 18, i32 26}
!11 = !{i32 -3, i32 -6, i32 2}
!12 = !{i32 -3, i32 -6, i32 26}
!13 = !{i32 -3, i32 -84, i32 364}
!14 = !{i32 -3, i32 -8, i32 26}
; CHECK: !15 = !{i32 -3, i32 -8, i32 26}
; CHECK: !16 = !{double 0x3FC30D3B4629C08B}