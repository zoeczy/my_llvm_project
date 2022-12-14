; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -S -passes=rewrite-statepoints-for-gc -rs4gc-clobber-non-live %s | FileCheck %s
; Make sure that clobber-non-live correctly handles vector types

define void @test_vector_clobber(i8 addrspace(1)* %ptr) gc "statepoint-example" {
; CHECK-LABEL: @test_vector_clobber(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[PTR_CAST:%.*]] = bitcast i8 addrspace(1)* [[PTR:%.*]] to float addrspace(1)*
; CHECK-NEXT:    [[STATEPOINT_TOKEN:%.*]] = call token (i64, i32, void ()*, i32, i32, ...) @llvm.experimental.gc.statepoint.p0f_isVoidf(i64 2882400000, i32 0, void ()* elementtype(void ()) @foo, i32 0, i32 0, i32 0, i32 0) [ "deopt"(i32 0, i32 2, i32 0, i32 62, i32 0, i32 13, i32 0, i32 7, i8* null, i32 7, i8* null, i32 7, i8* null, i32 3, i32 14, i32 3, i32 -2406, i32 3, i32 28963, i32 3, i32 30401, i32 3, i32 -11, i32 3, i32 -5, i32 3, i32 1, i32 0, i8 addrspace(1)* [[PTR]], i32 0, i8 addrspace(1)* [[PTR]], i32 7, i8* null), "gc-live"(i8 addrspace(1)* [[PTR]]) ]
; CHECK-NEXT:    [[PTR_RELOCATED:%.*]] = call coldcc i8 addrspace(1)* @llvm.experimental.gc.relocate.p1i8(token [[STATEPOINT_TOKEN]], i32 0, i32 0)
; CHECK-NEXT:    [[PTR_CAST_REMAT:%.*]] = bitcast i8 addrspace(1)* [[PTR_RELOCATED]] to float addrspace(1)*
; CHECK-NEXT:    [[CAST:%.*]] = bitcast i8 addrspace(1)* [[PTR_RELOCATED]] to float addrspace(1)*
; CHECK-NEXT:    [[DOTSPLATINSERT_BASE:%.*]] = insertelement <8 x float addrspace(1)*> zeroinitializer, float addrspace(1)* [[CAST]], i32 0, !is_base_value !0
; CHECK-NEXT:    [[DOTSPLATINSERT:%.*]] = insertelement <8 x float addrspace(1)*> poison, float addrspace(1)* [[PTR_CAST_REMAT]], i32 0
; CHECK-NEXT:    [[DOTSPLAT_BASE:%.*]] = shufflevector <8 x float addrspace(1)*> [[DOTSPLATINSERT_BASE]], <8 x float addrspace(1)*> undef, <8 x i32> zeroinitializer, !is_base_value !0
; CHECK-NEXT:    [[DOTSPLAT:%.*]] = shufflevector <8 x float addrspace(1)*> [[DOTSPLATINSERT]], <8 x float addrspace(1)*> poison, <8 x i32> zeroinitializer
; CHECK-NEXT:    [[GEP:%.*]] = getelementptr inbounds float, <8 x float addrspace(1)*> [[DOTSPLAT]], <8 x i64> <i64 83, i64 81, i64 79, i64 77, i64 75, i64 73, i64 71, i64 69>
; CHECK-NEXT:    [[STATEPOINT_TOKEN1:%.*]] = call token (i64, i32, void ()*, i32, i32, ...) @llvm.experimental.gc.statepoint.p0f_isVoidf(i64 2882400000, i32 0, void ()* elementtype(void ()) @bar, i32 0, i32 0, i32 0, i32 0) [ "deopt"(i32 0, i32 1, i32 0, i32 112, i32 0, i32 13, i32 0, i32 7, i8* null, i32 7, i8* null, i32 3, i32 undef, i32 3, i32 14, i32 3, i32 -2406, i32 3, i32 28963, i32 3, i32 30401, i32 3, i32 -11, i32 3, i32 -5, i32 3, i32 1, i32 0, i8 addrspace(1)* [[PTR_RELOCATED]], i32 0, i8 addrspace(1)* [[PTR_RELOCATED]], i32 7, i8* null), "gc-live"(<8 x float addrspace(1)*> [[GEP]], i8 addrspace(1)* [[PTR_RELOCATED]], <8 x float addrspace(1)*> [[DOTSPLAT_BASE]]) ]
; CHECK-NEXT:    [[GEP_RELOCATED:%.*]] = call coldcc <8 x i8 addrspace(1)*> @llvm.experimental.gc.relocate.v8p1i8(token [[STATEPOINT_TOKEN1]], i32 2, i32 0)
; CHECK-NEXT:    [[GEP_RELOCATED_CASTED:%.*]] = bitcast <8 x i8 addrspace(1)*> [[GEP_RELOCATED]] to <8 x float addrspace(1)*>
; CHECK-NEXT:    [[PTR_RELOCATED2:%.*]] = call coldcc i8 addrspace(1)* @llvm.experimental.gc.relocate.p1i8(token [[STATEPOINT_TOKEN1]], i32 1, i32 1)
; CHECK-NEXT:    [[DOTSPLAT_BASE_RELOCATED:%.*]] = call coldcc <8 x i8 addrspace(1)*> @llvm.experimental.gc.relocate.v8p1i8(token [[STATEPOINT_TOKEN1]], i32 2, i32 2)
; CHECK-NEXT:    [[DOTSPLAT_BASE_RELOCATED_CASTED:%.*]] = bitcast <8 x i8 addrspace(1)*> [[DOTSPLAT_BASE_RELOCATED]] to <8 x float addrspace(1)*>
; CHECK-NEXT:    [[RES:%.*]] = call <8 x float> @llvm.masked.gather.v8f32.v8p1f32(<8 x float addrspace(1)*> [[GEP_RELOCATED_CASTED]], i32 4, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <8 x float> undef)
; CHECK-NEXT:    unreachable
;
entry:
  %ptr.cast = bitcast i8 addrspace(1)* %ptr to float addrspace(1)*
  call void @foo() [ "deopt"(i32 0, i32 2, i32 0, i32 62, i32 0, i32 13, i32 0, i32 7, i8* null, i32 7, i8* null, i32 7, i8* null, i32 3, i32 14, i32 3, i32 -2406, i32 3, i32 28963, i32 3, i32 30401, i32 3, i32 -11, i32 3, i32 -5, i32 3, i32 1, i32 0, i8 addrspace(1)* %ptr, i32 0, i8 addrspace(1)* %ptr, i32 7, i8* null) ]
  %gep = getelementptr inbounds float, float addrspace(1)* %ptr.cast, <8 x i64> <i64 83, i64 81, i64 79, i64 77, i64 75, i64 73, i64 71, i64 69>
  call void @bar() [ "deopt"(i32 0, i32 1, i32 0, i32 112, i32 0, i32 13, i32 0, i32 7, i8* null, i32 7, i8* null, i32 3, i32 undef, i32 3, i32 14, i32 3, i32 -2406, i32 3, i32 28963, i32 3, i32 30401, i32 3, i32 -11, i32 3, i32 -5, i32 3, i32 1, i32 0, i8 addrspace(1)* %ptr, i32 0, i8 addrspace(1)* %ptr, i32 7, i8* null) ]
  %res = call <8 x float> @llvm.masked.gather.v8f32.v8p1f32(<8 x float addrspace(1)*> %gep, i32 4, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <8 x float> undef)
  unreachable
}


declare void @foo() gc "statepoint-example"

; Function Attrs: nocallback nofree nosync nounwind readonly willreturn
declare <8 x float> @llvm.masked.gather.v8f32.v8p1f32(<8 x float addrspace(1)*>, i32 immarg, <8 x i1>, <8 x float>) #1

declare void @bar()

attributes #1 = { nocallback nofree nosync nounwind readonly willreturn }
