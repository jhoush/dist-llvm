; RUN: llc < %s -mtriple=x86_64-apple-darwin -mcpu=corei7 | FileCheck %s
; rdar://7396984

@str = private constant [28 x i8] c"xxxxxxxxxxxxxxxxxxxxxxxxxxx\00", align 1

define void @t(i32 %count) ssp nounwind {
entry:
; CHECK: t:
; CHECK: movq ___stack_chk_guard@GOTPCREL(%rip)
; CHECK: movups L_str(%rip), %xmm0
  %tmp0 = alloca [60 x i8], align 1
  %tmp1 = getelementptr inbounds [60 x i8]* %tmp0, i64 0, i64 0
  br label %bb1

bb1:
; CHECK: LBB1_1:
; CHECK: movaps %xmm0, (%rsp)
  %tmp2 = phi i32 [ %tmp3, %bb1 ], [ 0, %entry ]
  call void @llvm.memcpy.i64(i8* %tmp1, i8* getelementptr inbounds ([28 x i8]* @str, i64 0, i64 0), i64 28, i32 1)
  %tmp3 = add i32 %tmp2, 1
  %tmp4 = icmp eq i32 %tmp3, %count
  br i1 %tmp4, label %bb2, label %bb1

bb2:
  ret void
}

declare void @llvm.memcpy.i64(i8* nocapture, i8* nocapture, i64, i32) nounwind
