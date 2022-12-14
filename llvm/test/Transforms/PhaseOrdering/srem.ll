; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -O1 -S < %s | FileCheck %s
; RUN: opt -O2 -S < %s | FileCheck %s
; RUN: opt -O3 -S < %s | FileCheck %s

define i32 @PR57472(i32 noundef %x) {
; CHECK-LABEL: @PR57472(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i32 [[X:%.*]], -1
; CHECK-NEXT:    [[REM:%.*]] = srem i32 [[X]], 16
; CHECK-NEXT:    [[COND:%.*]] = select i1 [[CMP]], i32 [[REM]], i32 42
; CHECK-NEXT:    ret i32 [[COND]]
;
entry:
  %x.addr = alloca i32, align 4
  store i32 %x, ptr %x.addr, align 4
  %0 = load i32, ptr %x.addr, align 4
  %cmp = icmp sge i32 %0, 0
  br i1 %cmp, label %cond.true, label %cond.false

cond.true:
  %1 = load i32, ptr %x.addr, align 4
  %rem = srem i32 %1, 16
  br label %cond.end

cond.false:
  br label %cond.end

cond.end:
  %cond = phi i32 [ %rem, %cond.true ], [ 42, %cond.false ]
  ret i32 %cond
}
