; RUN: opt -load %eputilslib -load %errorproplib -errorprop -S %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK: store i32 %a, i32* %a.addr, align 4, !errorprop.abserror !6
; CHECK: store i32 %b, i32* %b.addr, align 4, !errorprop.abserror !9
; CHECK: %0 = load i32, i32* %a.addr, align 4, !errorprop.abserror !6
; CHECK: %1 = load i32, i32* %b.addr, align 4, !errorprop.abserror !9
; CHECK: %add = add nsw i32 %0, %1, !errorprop.range !10, !errorprop.abserror !11
; CHECK: store i32 %add, i32* %a.addr, align 4, !errorprop.abserror !11
; CHECK: %2 = load i32, i32* %a.addr, align 4, !errorprop.abserror !11
; CHECK: %3 = load i32, i32* %b.addr, align 4, !errorprop.abserror !9
; CHECK: %sub = sub nsw i32 %2, %3, !errorprop.range !12, !errorprop.abserror !6
; CHECK: store i32 %sub, i32* %b.addr, align 4, !errorprop.abserror !6
; CHECK: %4 = load i32, i32* %a.addr, align 4, !errorprop.abserror !11
; CHECK: %5 = load i32, i32* %b.addr, align 4, !errorprop.abserror !6
; CHECK: %mul = mul nsw i32 %4, %5, !errorprop.range !13, !errorprop.abserror !14
; CHECK: store i32 %mul, i32* %a.addr, align 4, !errorprop.abserror !14
; CHECK: %6 = load i32, i32* %b.addr, align 4, !errorprop.abserror !6
; CHECK: %7 = load i32, i32* %a.addr, align 4, !errorprop.abserror !14
; CHECK: %div = sdiv i32 %6, %7, !errorprop.range !15, !errorprop.abserror !16
; CHECK: store i32 %div, i32* %b.addr, align 4, !errorprop.abserror !16
; CHECK: %8 = load i32, i32* %b.addr, align 4, !errorprop.abserror !16
; CHECK: ret i32 %8, !errorprop.abserror !16
; Function Attrs: noinline nounwind optnone uwtable
define i32 @foo(i32 %a, i32 %b) #0 !errorprop.propagatefunction !2 !errorprop.argsrange !3 {
entry:
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  store i32 %b, i32* %b.addr, align 4
  %0 = load i32, i32* %a.addr, align 4
  %1 = load i32, i32* %b.addr, align 4
  %add = add nsw i32 %0, %1, !errorprop.range !10
  store i32 %add, i32* %a.addr, align 4
  %2 = load i32, i32* %a.addr, align 4
  %3 = load i32, i32* %b.addr, align 4
  %sub = sub nsw i32 %2, %3, !errorprop.range !11
  store i32 %sub, i32* %b.addr, align 4
  %4 = load i32, i32* %a.addr, align 4
  %5 = load i32, i32* %b.addr, align 4
  %mul = mul nsw i32 %4, %5, !errorprop.range !12
  store i32 %mul, i32* %a.addr, align 4
  %6 = load i32, i32* %b.addr, align 4
  %7 = load i32, i32* %a.addr, align 4
  %div = sdiv i32 %6, %7, !errorprop.range !13
  store i32 %div, i32* %b.addr, align 4
  %8 = load i32, i32* %b.addr, align 4
  ret i32 %8
}

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.1 (https://git.llvm.org/git/clang.git/ 0e746072ed897a85b4f533ab050b9f506941a097) (git@github.com:llvm-mirror/llvm.git d31f0553233c351698688408021d45515023a98a)"}
!2 = !{i32 0}
!3 = !{!4, !7}
!4 = !{!5, !6}
!5 = !{i32 5, i32 8, i32 12}
!6 = !{double 1.000000e-02}
!7 = !{!8, !9}
!8 = !{i32 5, i32 10, i32 14}
!9 = !{double 2.000000e-03}
!10 = !{i32 5, i32 18, i32 26}
!11 = !{i32 5, i32 4, i32 16}
!12 = !{i32 5, i32 72, i32 416}
!13 = !{i32 5, i32 0, i32 0}

; CHECK: !11 = !{double 1.200000e-02}
; CHECK: !14 = !{double 1.424500e-02}
; CHECK: !16 = !{double 0x3FAA958E0C4188DD}
