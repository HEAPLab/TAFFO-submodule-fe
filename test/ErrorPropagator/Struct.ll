; RUN: opt -load %eputilslib -load %errorproplib -globals-aa -cfl-steens-aa -cfl-anders-aa -tbaa -errorprop -S %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.S = type { i32, i64 }

; CHECK: store i32 %s.coerce0, i32* %1, align 8, !errorprop.abserror !3
; CHECK: %2 = getelementptr inbounds { i32, i64 }, { i32, i64 }* %0, i32 0, i32 1
; CHECK: store i64 %s.coerce1, i64* %2, align 8, !errorprop.abserror !3
; CHECK: store i64 %c, i64* %c.addr, align 8, !errorprop.abserror !3
; CHECK: %3 = load i64, i64* %c.addr, align 8, !errorprop.abserror !3
; CHECK: %a = getelementptr inbounds %struct.S, %struct.S* %s, i32 0, i32 0
; CHECK: %4 = load i32, i32* %a, align 8, !errorprop.abserror !3
; CHECK: %conv = sext i32 %4 to i64, !errorprop.abserror !3
; CHECK: %add = add nsw i64 %conv, %3, !errorprop.range !8, !errorprop.abserror !9
; CHECK: %conv1 = trunc i64 %add to i32, !errorprop.abserror !9
; CHECK: store i32 %conv1, i32* %a, align 8, !errorprop.abserror !9
; CHECK: %5 = load i64, i64* %c.addr, align 8, !errorprop.abserror !3
; CHECK: %b = getelementptr inbounds %struct.S, %struct.S* %s, i32 0, i32 1
; CHECK: %6 = load i64, i64* %b, align 8, !errorprop.abserror !3
; CHECK: %mul = mul nsw i64 %6, %5, !errorprop.range !10, !errorprop.abserror !11
; CHECK: store i64 %mul, i64* %b, align 8, !errorprop.abserror !11
; CHECK: %a2 = getelementptr inbounds %struct.S, %struct.S* %s, i32 0, i32 0
; CHECK: %7 = load i32, i32* %a2, align 8, !errorprop.abserror !9
; CHECK: %conv3 = sext i32 %7 to i64, !errorprop.abserror !9
; CHECK: %b4 = getelementptr inbounds %struct.S, %struct.S* %s, i32 0, i32 1
; CHECK: %8 = load i64, i64* %b4, align 8, !errorprop.abserror !11
; CHECK: %add5 = add nsw i64 %conv3, %8, !errorprop.range !12, !errorprop.abserror !13
; CHECK: %conv6 = trunc i64 %add5 to i32, !errorprop.abserror !13
; CHECK: ret i32 %conv6, !errorprop.abserror !13

define i32 @foo(i32 %s.coerce0, i64 %s.coerce1, i64 %c) !errorprop.argsrange !0 {
entry:
  %s = alloca %struct.S, align 8
  %c.addr = alloca i64, align 8
  %0 = bitcast %struct.S* %s to { i32, i64 }*
  %1 = getelementptr inbounds { i32, i64 }, { i32, i64 }* %0, i32 0, i32 0
  store i32 %s.coerce0, i32* %1, align 8
  %2 = getelementptr inbounds { i32, i64 }, { i32, i64 }* %0, i32 0, i32 1
  store i64 %s.coerce1, i64* %2, align 8
  store i64 %c, i64* %c.addr, align 8
  %3 = load i64, i64* %c.addr, align 8
  %a = getelementptr inbounds %struct.S, %struct.S* %s, i32 0, i32 0
  %4 = load i32, i32* %a, align 8
  %conv = sext i32 %4 to i64
  %add = add nsw i64 %conv, %3, !errorprop.range !8
  %conv1 = trunc i64 %add to i32
  store i32 %conv1, i32* %a, align 8
  %5 = load i64, i64* %c.addr, align 8
  %b = getelementptr inbounds %struct.S, %struct.S* %s, i32 0, i32 1
  %6 = load i64, i64* %b, align 8
  %mul = mul nsw i64 %6, %5, !errorprop.range !9
  store i64 %mul, i64* %b, align 8
  %a2 = getelementptr inbounds %struct.S, %struct.S* %s, i32 0, i32 0
  %7 = load i32, i32* %a2, align 8
  %conv3 = sext i32 %7 to i64
  %b4 = getelementptr inbounds %struct.S, %struct.S* %s, i32 0, i32 1
  %8 = load i64, i64* %b4, align 8
  %add5 = add nsw i64 %conv3, %8, !errorprop.range !10
  %conv6 = trunc i64 %add5 to i32
  ret i32 %conv6
}

!0 = !{!1, !4, !6}
!1 = !{!2, !3}
!2 = !{i32 -15, i32 -163840, i32 327680}
!3 = !{double 2.000000e-05}
!4 = !{!5, !3}
!5 = !{i32 -15, i64 -327680, i64 655360}
!6 = !{!7, !3}
!7 = !{i32 -15, i64 -655360, i64 491520}
!8 = !{i32 -15, i64 -819200, i64 819200}
!9 = !{i32 -15, i64 -13107200, i64 9830400}
!10 = !{i32 -15, i64 -13926400, i64 10649600}

; CHECK: !9 = !{double 4.000000e-05}
; CHECK: !11 = !{double 0x3F4A36E3C70341FC}
; CHECK: !13 = !{double 0x3F4B866F1F91788B}