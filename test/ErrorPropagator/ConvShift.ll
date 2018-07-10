; RUN: opt -load %eputilslib -load %errorproplib -errorprop -S %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [17 x i8] c"hello world, %d\0A\00", align 1

; CHECK: store i32 10, i32* %a, align 4, !errorprop.range !3, !errorprop.abserror !4
; CHECK: store i32 11, i32* %b, align 4, !errorprop.range !5, !errorprop.abserror !4
; CHECK: %0 = load i32, i32* %a, align 4, !errorprop.abserror !4
; CHECK: %1 = load i32, i32* %b, align 4, !errorprop.abserror !4
; CHECK: %add = add nsw i32 %0, %1, !errorprop.range !6, !errorprop.abserror !7
; CHECK: store i32 %add, i32* %a, align 4, !errorprop.abserror !7
; CHECK: %2 = load i32, i32* %a, align 4, !errorprop.abserror !7
; CHECK: %shl = shl i32 %2, 3, !errorprop.range !8, !errorprop.abserror !7
; CHECK: store i32 %shl, i32* %a, align 4, !errorprop.abserror !7
; CHECK: %3 = load i32, i32* %a, align 4, !errorprop.abserror !7
; CHECK: %4 = load i32, i32* %b, align 4, !errorprop.abserror !4
; CHECK: %sub = sub nsw i32 %3, %4, !errorprop.range !9, !errorprop.abserror !4
; CHECK: store i32 %sub, i32* %b, align 4, !errorprop.abserror !4
; CHECK: %5 = load i32, i32* %b, align 4, !errorprop.abserror !4
; CHECK: %shr = ashr i32 %5, 3, !errorprop.range !10, !errorprop.abserror !11
; CHECK: store i32 %shr, i32* %b, align 4, !errorprop.abserror !11
; CHECK: %6 = load i32, i32* %a, align 4, !errorprop.abserror !7
; CHECK: %7 = load i32, i32* %b, align 4, !errorprop.abserror !11
; CHECK: %mul = mul nsw i32 %6, %7, !errorprop.range !12, !errorprop.abserror !13
; CHECK: store i32 %mul, i32* %a, align 4, !errorprop.abserror !13
; CHECK: %8 = load i32, i32* %b, align 4, !errorprop.abserror !11
; CHECK: %9 = load i32, i32* %a, align 4, !errorprop.abserror !13
; CHECK: %div = sdiv i32 %8, %9, !errorprop.range !14, !errorprop.abserror !15
; CHECK: store i32 %div, i32* %b, align 4, !errorprop.abserror !15
; CHECK: %10 = load i32, i32* %b, align 4, !errorprop.abserror !15
; Function Attrs: noinline nounwind optnone uwtable
define i32 @main() !errorprop.argsrange !{} {
entry:
  %retval = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  store i32 0, i32* %retval, align 4
  store i32 10, i32* %a, align 4, !errorprop.range !2
  store i32 11, i32* %b, align 4, !errorprop.range !3
  %0 = load i32, i32* %a, align 4
  %1 = load i32, i32* %b, align 4
  %add = add nsw i32 %0, %1, !errorprop.range !4
  store i32 %add, i32* %a, align 4
  %2 = load i32, i32* %a, align 4
  %shl = shl i32 %2, 3, !errorprop.range !8
  store i32 %shl, i32* %a, align 4
  %3 = load i32, i32* %a, align 4
  %4 = load i32, i32* %b, align 4
  %sub = sub nsw i32 %3, %4, !errorprop.range !5
  store i32 %sub, i32* %b, align 4
  %5 = load i32, i32* %b, align 4
  %shr = ashr i32 %5, 3, !errorprop.range !9
  store i32 %shr, i32* %b, align 4
  %6 = load i32, i32* %a, align 4
  %7 = load i32, i32* %b, align 4
  %mul = mul nsw i32 %6, %7, !errorprop.range !6
  store i32 %mul, i32* %a, align 4
  %8 = load i32, i32* %b, align 4
  %9 = load i32, i32* %a, align 4
  %div = sdiv i32 %8, %9, !errorprop.range !7
  store i32 %div, i32* %b, align 4
  %10 = load i32, i32* %b, align 4
  %call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @.str, i32 0, i32 0), i32 %10)
  ret i32 0
}

declare i32 @printf(i8*, ...)

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.1 (https://git.llvm.org/git/clang.git/ 0e746072ed897a85b4f533ab050b9f506941a097) (git@github.com:llvm-mirror/llvm.git c36325f946ec29943d593d6b33664cb564df99ea)"}
!2 = !{i32 5, i32 8, i32 12}
!3 = !{i32 5, i32 10, i32 14}
!4 = !{i32 5, i32 18, i32 26}
!5 = !{i32 5, i32 4, i32 16}
!6 = !{i32 5, i32 72, i32 416}
!7 = !{i32 5, i32 0, i32 0}
!8 = !{i32 8, i32 18, i32 26}
!9 = !{i32 2, i32 4, i32 16}

; CHECK: !4 = !{double 3.125000e-02}
; CHECK: !7 = !{double 6.250000e-02}
; CHECK: !11 = !{double 2.812500e-01}
; CHECK: !13 = !{double 0x3FD2F40000000000}
; CHECK: !15 = !{double 0x3FE065121211F59A}
; CHECK: !16 = !{double 0.000000e+00}
