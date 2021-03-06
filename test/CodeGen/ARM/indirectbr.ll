; RUN: llc < %s -relocation-model=pic -mtriple=arm-apple-darwin | FileCheck %s -check-prefix=ARM
; RUN: llc < %s -relocation-model=pic -mtriple=thumb-apple-darwin | FileCheck %s -check-prefix=THUMB
; RUN: llc < %s -relocation-model=static -mtriple=thumbv7-apple-darwin | FileCheck %s -check-prefix=THUMB2

@nextaddr = global i8* null                       ; <i8**> [#uses=2]
@C.0.2070 = private constant [5 x i8*] [i8* blockaddress(@foo, %L1), i8* blockaddress(@foo, %L2), i8* blockaddress(@foo, %L3), i8* blockaddress(@foo, %L4), i8* blockaddress(@foo, %L5)] ; <[5 x i8*]*> [#uses=1]

define internal arm_apcscc i32 @foo(i32 %i) nounwind {
; ARM: foo:
; THUMB: foo:
; THUMB2: foo:
entry:
  %0 = load i8** @nextaddr, align 4               ; <i8*> [#uses=2]
  %1 = icmp eq i8* %0, null                       ; <i1> [#uses=1]
; indirect branch gets duplicated here
; ARM: bx
; THUMB: mov pc, r1
; THUMB2: mov pc, r2
  br i1 %1, label %bb3, label %bb2

bb2:                                              ; preds = %entry, %bb3
  %gotovar.4.0 = phi i8* [ %gotovar.4.0.pre, %bb3 ], [ %0, %entry ] ; <i8*> [#uses=1]
; ARM: bx
; THUMB: mov pc, r1
; THUMB2: mov pc, r2
  indirectbr i8* %gotovar.4.0, [label %L5, label %L4, label %L3, label %L2, label %L1]

bb3:                                              ; preds = %entry
  %2 = getelementptr inbounds [5 x i8*]* @C.0.2070, i32 0, i32 %i ; <i8**> [#uses=1]
  %gotovar.4.0.pre = load i8** %2, align 4        ; <i8*> [#uses=1]
  br label %bb2

L5:                                               ; preds = %bb2
  br label %L4

L4:                                               ; preds = %L5, %bb2
  %res.0 = phi i32 [ 385, %L5 ], [ 35, %bb2 ]     ; <i32> [#uses=1]
  br label %L3

L3:                                               ; preds = %L4, %bb2
  %res.1 = phi i32 [ %res.0, %L4 ], [ 5, %bb2 ]   ; <i32> [#uses=1]
  br label %L2

L2:                                               ; preds = %L3, %bb2
  %res.2 = phi i32 [ %res.1, %L3 ], [ 1, %bb2 ]   ; <i32> [#uses=1]
  %phitmp = mul i32 %res.2, 6                     ; <i32> [#uses=1]
  br label %L1

L1:                                               ; preds = %L2, %bb2
  %res.3 = phi i32 [ %phitmp, %L2 ], [ 2, %bb2 ]  ; <i32> [#uses=1]
; ARM: ldr r1, LCPI
; ARM: add r1, pc, r1
; ARM: str r1
; THUMB: ldr.n r2, LCPI
; THUMB: add r2, pc
; THUMB: str r2
; THUMB2: ldr.n r2, LCPI
; THUMB2-NEXT: str r2
  store i8* blockaddress(@foo, %L5), i8** @nextaddr, align 4
  ret i32 %res.3
}
; ARM: .long Ltmp0-(LPC{{.*}}+8)
; THUMB: .long Ltmp0-(LPC{{.*}}+4)
; THUMB2: .long Ltmp0
