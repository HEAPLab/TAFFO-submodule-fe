; RUN: opt -load %errorproplib -globals-aa -cfl-steens-aa -cfl-anders-aa -tbaa -errorprop -S %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.S = type { i32, i64 }

; CHECK: store i32 %s.coerce0, i32* %1, align 8, !taffo.abserror !4
; CHECK: store i64 %s.coerce1, i64* %2, align 8, !taffo.abserror !4
; CHECK: store i64 %c, i64* %c.addr, align 8, !taffo.abserror !4
; CHECK: %3 = load i64, i64* %c.addr, align 8, !taffo.abserror !4
; CHECK: %4 = load i32, i32* %a, align 8, !taffo.abserror !4
; CHECK: %conv = sext i32 %4 to i64, !taffo.abserror !4
; CHECK: %add = add nsw i64 %conv, %3, !taffo.info !10, !taffo.abserror !12
; CHECK: %conv1 = trunc i64 %add to i32, !taffo.abserror !12
; CHECK: store i32 %conv1, i32* %a, align 8, !taffo.abserror !12
; CHECK: %5 = load i64, i64* %c.addr, align 8, !taffo.abserror !4
; CHECK: %6 = load i64, i64* %b, align 8, !taffo.abserror !4
; CHECK: %mul = mul nsw i64 %6, %5, !taffo.info !13, !taffo.abserror !15
; CHECK: store i64 %mul, i64* %b, align 8, !taffo.abserror !15
; CHECK: %7 = load i32, i32* %a2, align 8, !taffo.abserror !12
; CHECK: %conv3 = sext i32 %7 to i64, !taffo.abserror !12
; CHECK: %8 = load i64, i64* %b4, align 8, !taffo.abserror !15
; CHECK: %add5 = add nsw i64 %conv3, %8, !taffo.info !16, !taffo.abserror !18
; CHECK: %conv6 = trunc i64 %add5 to i32, !taffo.abserror !18
; CHECK: ret i32 %conv6, !taffo.abserror !18

define i32 @foo(i32 %s.coerce0, i64 %s.coerce1, i64 %c) !taffo.funinfo !0 {
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
  %add = add nsw i64 %conv, %3, !taffo.info !9
  %conv1 = trunc i64 %add to i32
  store i32 %conv1, i32* %a, align 8
  %5 = load i64, i64* %c.addr, align 8
  %b = getelementptr inbounds %struct.S, %struct.S* %s, i32 0, i32 1
  %6 = load i64, i64* %b, align 8
  %mul = mul nsw i64 %6, %5, !taffo.info !11
  store i64 %mul, i64* %b, align 8
  %a2 = getelementptr inbounds %struct.S, %struct.S* %s, i32 0, i32 0
  %7 = load i32, i32* %a2, align 8
  %conv3 = sext i32 %7 to i64
  %b4 = getelementptr inbounds %struct.S, %struct.S* %s, i32 0, i32 1
  %8 = load i64, i64* %b4, align 8
  %add5 = add nsw i64 %conv3, %8, !taffo.info !13
  %conv6 = trunc i64 %add5 to i32
  ret i32 %conv6
}

!0 = !{!1, !5, !7}
!1 = !{!15, !3, !4}
!2 = !{!"fixp", i32 -64, i32 15}
!3 = !{double -5.000000e+00, double 1.000000e+01}
!4 = !{double 2.000000e-05}
!5 = !{!2, !6, !4}
!6 = !{double -1.000000e+01, double 2.000000e+01}
!7 = !{!2, !8, !4}
!8 = !{double -2.000000e+01, double 1.500000e+01}
!9 = !{!2, !10, i1 0}
!10 = !{double -2.500000e+01, double 2.500000e+01}
!11 = !{!2, !12, i1 0}
!12 = !{double -4.000000e+02, double 3.000000e+02}
!13 = !{!2, !14, i1 0}
!14 = !{double -4.250000e+02, double 3.250000e+02}
!15 = !{!"fixp", i32 -32, i32 15}

; CHECK: !12 = !{double 4.000000e-05}
; CHECK: !15 = !{double 0x3F4A36E3C70341FC}
; CHECK: !18 = !{double 0x3F4B866F1F91788B}
