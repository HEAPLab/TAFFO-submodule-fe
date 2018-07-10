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
; CHECK: %conv = sext i32 %2 to i64, !errorprop.range !8, !errorprop.abserror !7
; CHECK: store i64 %conv, i64* %c, align 8, !errorprop.abserror !7
; CHECK: %3 = load i32, i32* %b, align 4, !errorprop.abserror !4
; CHECK: %conv1 = sext i32 %3 to i64, !errorprop.range !9, !errorprop.abserror !4
; CHECK: store i64 %conv1, i64* %d, align 8, !errorprop.abserror !4
; CHECK: %4 = load i64, i64* %c, align 8, !errorprop.abserror !7
; CHECK: %5 = load i64, i64* %d, align 8, !errorprop.abserror !4
; CHECK: %sub = sub nsw i64 %4, %5, !errorprop.range !10, !errorprop.abserror !4
; CHECK: %conv2 = trunc i64 %sub to i32, !errorprop.range !11, !errorprop.abserror !4
; CHECK: store i32 %conv2, i32* %b, align 4, !errorprop.abserror !4
; CHECK: %6 = load i32, i32* %b, align 4, !errorprop.abserror !4
; Function Attrs: noinline nounwind optnone uwtable
define i32 @main() #0 !errorprop.argsrange !2 {
entry:
  %retval = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i64, align 8
  %d = alloca i64, align 8
  store i32 0, i32* %retval, align 4
  store i32 10, i32* %a, align 4, !errorprop.range !3
  store i32 11, i32* %b, align 4, !errorprop.range !4
  %0 = load i32, i32* %a, align 4
  %1 = load i32, i32* %b, align 4
  %add = add nsw i32 %0, %1, !errorprop.range !5
  store i32 %add, i32* %a, align 4
  %2 = load i32, i32* %a, align 4
  %conv = sext i32 %2 to i64, !errorprop.range !7
  store i64 %conv, i64* %c, align 8
  %3 = load i32, i32* %b, align 4
  %conv1 = sext i32 %3 to i64, !errorprop.range !8
  store i64 %conv1, i64* %d, align 8
  %4 = load i64, i64* %c, align 8
  %5 = load i64, i64* %d, align 8
  %sub = sub nsw i64 %4, %5, !errorprop.range !6
  %conv2 = trunc i64 %sub to i32, !errorprop.range !9
  store i32 %conv2, i32* %b, align 4
  %6 = load i32, i32* %b, align 4
  %call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @.str, i32 0, i32 0), i32 %6)
  ret i32 0
}

declare i32 @printf(i8*, ...) #1

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.1 (https://git.llvm.org/git/clang.git/ 0e746072ed897a85b4f533ab050b9f506941a097) (git@github.com:llvm-mirror/llvm.git c36325f946ec29943d593d6b33664cb564df99ea)"}
!2 = !{}
!3 = !{i32 5, i32 8, i32 12}
!4 = !{i32 5, i32 10, i32 14}
!5 = !{i32 5, i32 18, i32 26}
!6 = !{i32 5, i32 4, i32 16}
!7 = !{i32 5, i64 18, i64 26}
!8 = !{i32 5, i64 10, i64 14}
!9 = !{i32 5, i64 4, i64 16}

; CHECK: !4 = !{double 3.125000e-02}
; CHECK: !7 = !{double 6.250000e-02}
