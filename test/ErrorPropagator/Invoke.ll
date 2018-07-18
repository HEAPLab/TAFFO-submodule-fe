; RUN: opt -load %eputilslib -load %errorproplib -errorprop -S %s | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK: store i64 %x, i64* %x.addr, align 8, !errorprop.abserror !3
; CHECK: %0 = load i64, i64* %x.addr, align 8, !errorprop.abserror !3
; CHECK: %1 = load i64, i64* %x.addr, align 8, !errorprop.abserror !3
; CHECK: %mul = mul nsw i64 %0, %1, !errorprop.range !4, !errorprop.abserror !5
; CHECK: %2 = load i64, i64* %x.addr, align 8, !errorprop.abserror !3
; CHECK: %mul1 = mul nsw i64 %mul, %2, !errorprop.range !6, !errorprop.abserror !7
; CHECK: ret i64 %mul1, !errorprop.abserror !7

; Function Attrs: noinline nounwind optnone uwtable
define i64 @_Z3barl(i64 %x) #0 !errorprop.argsrange !0 {
entry:
  %x.addr = alloca i64, align 8
  store i64 %x, i64* %x.addr, align 8
  %0 = load i64, i64* %x.addr, align 8
  %1 = load i64, i64* %x.addr, align 8
  %mul = mul nsw i64 %0, %1, !errorprop.range !4
  %2 = load i64, i64* %x.addr, align 8
  %mul1 = mul nsw i64 %mul, %2, !errorprop.range !5
  ret i64 %mul1
}

; CHECK: store i64 %a, i64* %a.addr, align 8, !errorprop.abserror !11
; CHECK: store i64 %b, i64* %b.addr, align 8, !errorprop.abserror !14
; CHECK: %0 = load i64, i64* %a.addr, align 8, !errorprop.abserror !11
; CHECK: %1 = load i64, i64* %b.addr, align 8, !errorprop.abserror !14
; CHECK: %mul = mul nsw i64 %0, %1, !errorprop.range !15, !errorprop.abserror !16
; CHECK: to label %invoke.cont unwind label %lpad, !errorprop.range !6, !errorprop.abserror !17
; CHECK: store i64 %call, i64* %retval, align 8, !errorprop.abserror !17
; CHECK: %6 = load i64, i64* %a.addr, align 8, !errorprop.abserror !11
; CHECK: %7 = load i64, i64* %b.addr, align 8, !errorprop.abserror !14
; CHECK: %mul1 = mul nsw i64 %6, %7, !errorprop.range !15, !errorprop.abserror !16
; CHECK: store i64 %mul1, i64* %retval, align 8, !errorprop.abserror !16

; Function Attrs: noinline optnone uwtable
define i64 @_Z3fooll(i64 %a, i64 %b) #1 personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*) !errorprop.argsrange !6 {
entry:
  %retval = alloca i64, align 8
  %a.addr = alloca i64, align 8
  %b.addr = alloca i64, align 8
  %exn.slot = alloca i8*
  %ehselector.slot = alloca i32
  store i64 %a, i64* %a.addr, align 8
  store i64 %b, i64* %b.addr, align 8
  %0 = load i64, i64* %a.addr, align 8
  %1 = load i64, i64* %b.addr, align 8
  %mul = mul nsw i64 %0, %1, !errorprop.range !13
  %call = invoke i64 @_Z3barl(i64 %mul)
          to label %invoke.cont unwind label %lpad, !errorprop.range !5

invoke.cont:                                      ; preds = %entry
  store i64 %call, i64* %retval, align 8
  br label %return

lpad:                                             ; preds = %entry
  %2 = landingpad { i8*, i32 }
          catch i8* null
  %3 = extractvalue { i8*, i32 } %2, 0
  store i8* %3, i8** %exn.slot, align 8
  %4 = extractvalue { i8*, i32 } %2, 1
  store i32 %4, i32* %ehselector.slot, align 4
  br label %catch

catch:                                            ; preds = %lpad
  %exn = load i8*, i8** %exn.slot, align 8
  %5 = call i8* @__cxa_begin_catch(i8* %exn) #3
  %6 = load i64, i64* %a.addr, align 8
  %7 = load i64, i64* %b.addr, align 8
  %mul1 = mul nsw i64 %6, %7, !errorprop.range !13
  store i64 %mul1, i64* %retval, align 8
  call void @__cxa_end_catch()
  br label %return

try.cont:                                         ; No predecessors!
  call void @llvm.trap()
  unreachable

return:                                           ; preds = %catch, %invoke.cont
  %8 = load i64, i64* %retval, align 8
  ret i64 %8
}

declare i32 @__gxx_personality_v0(...)

declare i8* @__cxa_begin_catch(i8*)

declare void @__cxa_end_catch()

; Function Attrs: noreturn nounwind
declare void @llvm.trap() #2

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { noinline optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { noreturn nounwind }
attributes #3 = { nounwind }

!0 = !{!1}
!1 = !{!2, !3}
!2 = !{i32 32, i64 12884901888, i64 47244640256}
!3 = !{double 4.000000e-10}
!4 = !{i32 32, i64 38654705664, i64 519691042816}
!5 = !{i32 32, i64 115964116992, i64 5716601470976}
!6 = !{!7, !8}
!7 = !{!9, !10}
!9 = !{i32 32, i64 0, i64 85899345920}
!10 = !{double 5.000000e-10}
!8 = !{!11, !12}
!11 = !{i32 32, i64 4294967296, i64 8589934592}
!12 = !{double 6.000000e-10}
!13 = !{i32 32, i64 0, i64 171798691840}

; CHECK: !5 = !{double 0x3E42E5D9E5C5CC3B}
; CHECK: !7 = !{double 0x3E837D08B4F58035}
; CHECK: !16 = !{double 0x3E4BEAD35941E10C}
; CHECK: !17 = !{double 0x3ED3CAFCD82CAC6E}
