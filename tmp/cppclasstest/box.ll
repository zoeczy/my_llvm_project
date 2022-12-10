; ModuleID = 'box.cpp'
source_filename = "box.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%"class.std::ios_base::Init" = type { i8 }
%"class.std::basic_ostream" = type { ptr, %"class.std::basic_ios" }
%"class.std::basic_ios" = type { %"class.std::ios_base", ptr, i8, i8, ptr, ptr, ptr, ptr }
%"class.std::ios_base" = type { ptr, i64, i64, i32, i32, i32, ptr, %"struct.std::ios_base::_Words", [8 x %"struct.std::ios_base::_Words"], i32, ptr, %"class.std::locale" }
%"struct.std::ios_base::_Words" = type { ptr, i64 }
%"class.std::locale" = type { ptr }
%class.Box = type { double, double, double }

@_ZStL8__ioinit = internal global %"class.std::ios_base::Init" zeroinitializer, align 1
@__dso_handle = external hidden global i8
@_ZSt4cout = external global %"class.std::basic_ostream", align 8
@.str = private unnamed_addr constant [18 x i8] c"Box1 \E7\9A\84\E4\BD\93\E7\A7\AF\EF\BC\9A\00", align 1
@.str.1 = private unnamed_addr constant [18 x i8] c"Box2 \E7\9A\84\E4\BD\93\E7\A7\AF\EF\BC\9A\00", align 1
@.str.2 = private unnamed_addr constant [18 x i8] c"Box3 \E7\9A\84\E4\BD\93\E7\A7\AF\EF\BC\9A\00", align 1
@llvm.global_ctors = appending global [1 x { i32, ptr, ptr }] [{ i32, ptr, ptr } { i32 65535, ptr @_GLOBAL__sub_I_box.cpp, ptr null }]

; Function Attrs: noinline uwtable
define internal void @__cxx_global_var_init() #0 section ".text.startup" {
entry:
  call void @_ZNSt8ios_base4InitC1Ev(ptr noundef nonnull align 1 dereferenceable(1) @_ZStL8__ioinit)
  %0 = call i32 @__cxa_atexit(ptr @_ZNSt8ios_base4InitD1Ev, ptr @_ZStL8__ioinit, ptr @__dso_handle) #3
  ret void
}

declare void @_ZNSt8ios_base4InitC1Ev(ptr noundef nonnull align 1 dereferenceable(1)) unnamed_addr #1

; Function Attrs: nounwind
declare void @_ZNSt8ios_base4InitD1Ev(ptr noundef nonnull align 1 dereferenceable(1)) unnamed_addr #2

; Function Attrs: nounwind
declare i32 @__cxa_atexit(ptr, ptr, ptr) #3

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define dso_local noundef double @_ZN3Box3getEv(ptr noundef nonnull align 8 dereferenceable(24) %this) #4 align 2 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  %this1 = load ptr, ptr %this.addr, align 8
  %length = getelementptr inbounds %class.Box, ptr %this1, i32 0, i32 0
  %0 = load double, ptr %length, align 8
  %breadth = getelementptr inbounds %class.Box, ptr %this1, i32 0, i32 1
  %1 = load double, ptr %breadth, align 8
  %mul = fmul double %0, %1
  %height = getelementptr inbounds %class.Box, ptr %this1, i32 0, i32 2
  %2 = load double, ptr %height, align 8
  %mul2 = fmul double %mul, %2
  ret double %mul2
}

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define dso_local void @_ZN3Box3setEddd(ptr noundef nonnull align 8 dereferenceable(24) %this, double noundef %len, double noundef %bre, double noundef %hei) #4 align 2 {
entry:
  %this.addr = alloca ptr, align 8
  %len.addr = alloca double, align 8
  %bre.addr = alloca double, align 8
  %hei.addr = alloca double, align 8
  store ptr %this, ptr %this.addr, align 8
  store double %len, ptr %len.addr, align 8
  store double %bre, ptr %bre.addr, align 8
  store double %hei, ptr %hei.addr, align 8
  %this1 = load ptr, ptr %this.addr, align 8
  %0 = load double, ptr %len.addr, align 8
  %length = getelementptr inbounds %class.Box, ptr %this1, i32 0, i32 0
  store double %0, ptr %length, align 8
  %1 = load double, ptr %bre.addr, align 8
  %breadth = getelementptr inbounds %class.Box, ptr %this1, i32 0, i32 1
  store double %1, ptr %breadth, align 8
  %2 = load double, ptr %hei.addr, align 8
  %height = getelementptr inbounds %class.Box, ptr %this1, i32 0, i32 2
  store double %2, ptr %height, align 8
  ret void
}

; Function Attrs: mustprogress noinline norecurse optnone uwtable
define dso_local noundef i32 @main() #5 {
entry:
  %retval = alloca i32, align 4
  %Box1 = alloca %class.Box, align 8
  %Box2 = alloca %class.Box, align 8
  %Box3 = alloca %class.Box, align 8
  %volume = alloca double, align 8
  store i32 0, ptr %retval, align 4
  store double 0.000000e+00, ptr %volume, align 8
  %height = getelementptr inbounds %class.Box, ptr %Box1, i32 0, i32 2
  store double 5.000000e+00, ptr %height, align 8
  %length = getelementptr inbounds %class.Box, ptr %Box1, i32 0, i32 0
  store double 6.000000e+00, ptr %length, align 8
  %breadth = getelementptr inbounds %class.Box, ptr %Box1, i32 0, i32 1
  store double 7.000000e+00, ptr %breadth, align 8
  %height1 = getelementptr inbounds %class.Box, ptr %Box2, i32 0, i32 2
  store double 1.000000e+01, ptr %height1, align 8
  %length2 = getelementptr inbounds %class.Box, ptr %Box2, i32 0, i32 0
  store double 1.200000e+01, ptr %length2, align 8
  %breadth3 = getelementptr inbounds %class.Box, ptr %Box2, i32 0, i32 1
  store double 1.300000e+01, ptr %breadth3, align 8
  %height4 = getelementptr inbounds %class.Box, ptr %Box1, i32 0, i32 2
  %0 = load double, ptr %height4, align 8
  %length5 = getelementptr inbounds %class.Box, ptr %Box1, i32 0, i32 0
  %1 = load double, ptr %length5, align 8
  %mul = fmul double %0, %1
  %breadth6 = getelementptr inbounds %class.Box, ptr %Box1, i32 0, i32 1
  %2 = load double, ptr %breadth6, align 8
  %mul7 = fmul double %mul, %2
  store double %mul7, ptr %volume, align 8
  %call = call noundef nonnull align 8 dereferenceable(8) ptr @_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc(ptr noundef nonnull align 8 dereferenceable(8) @_ZSt4cout, ptr noundef @.str)
  %3 = load double, ptr %volume, align 8
  %call8 = call noundef nonnull align 8 dereferenceable(8) ptr @_ZNSolsEd(ptr noundef nonnull align 8 dereferenceable(8) %call, double noundef %3)
  %call9 = call noundef nonnull align 8 dereferenceable(8) ptr @_ZNSolsEPFRSoS_E(ptr noundef nonnull align 8 dereferenceable(8) %call8, ptr noundef @_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_)
  %height10 = getelementptr inbounds %class.Box, ptr %Box2, i32 0, i32 2
  %4 = load double, ptr %height10, align 8
  %length11 = getelementptr inbounds %class.Box, ptr %Box2, i32 0, i32 0
  %5 = load double, ptr %length11, align 8
  %mul12 = fmul double %4, %5
  %breadth13 = getelementptr inbounds %class.Box, ptr %Box2, i32 0, i32 1
  %6 = load double, ptr %breadth13, align 8
  %mul14 = fmul double %mul12, %6
  store double %mul14, ptr %volume, align 8
  %call15 = call noundef nonnull align 8 dereferenceable(8) ptr @_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc(ptr noundef nonnull align 8 dereferenceable(8) @_ZSt4cout, ptr noundef @.str.1)
  %7 = load double, ptr %volume, align 8
  %call16 = call noundef nonnull align 8 dereferenceable(8) ptr @_ZNSolsEd(ptr noundef nonnull align 8 dereferenceable(8) %call15, double noundef %7)
  %call17 = call noundef nonnull align 8 dereferenceable(8) ptr @_ZNSolsEPFRSoS_E(ptr noundef nonnull align 8 dereferenceable(8) %call16, ptr noundef @_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_)
  call void @_ZN3Box3setEddd(ptr noundef nonnull align 8 dereferenceable(24) %Box3, double noundef 1.600000e+01, double noundef 8.000000e+00, double noundef 1.200000e+01)
  %call18 = call noundef double @_ZN3Box3getEv(ptr noundef nonnull align 8 dereferenceable(24) %Box3)
  store double %call18, ptr %volume, align 8
  %call19 = call noundef nonnull align 8 dereferenceable(8) ptr @_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc(ptr noundef nonnull align 8 dereferenceable(8) @_ZSt4cout, ptr noundef @.str.2)
  %8 = load double, ptr %volume, align 8
  %call20 = call noundef nonnull align 8 dereferenceable(8) ptr @_ZNSolsEd(ptr noundef nonnull align 8 dereferenceable(8) %call19, double noundef %8)
  %call21 = call noundef nonnull align 8 dereferenceable(8) ptr @_ZNSolsEPFRSoS_E(ptr noundef nonnull align 8 dereferenceable(8) %call20, ptr noundef @_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_)
  ret i32 0
}

declare noundef nonnull align 8 dereferenceable(8) ptr @_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc(ptr noundef nonnull align 8 dereferenceable(8), ptr noundef) #1

declare noundef nonnull align 8 dereferenceable(8) ptr @_ZNSolsEd(ptr noundef nonnull align 8 dereferenceable(8), double noundef) #1

declare noundef nonnull align 8 dereferenceable(8) ptr @_ZNSolsEPFRSoS_E(ptr noundef nonnull align 8 dereferenceable(8), ptr noundef) #1

declare noundef nonnull align 8 dereferenceable(8) ptr @_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_(ptr noundef nonnull align 8 dereferenceable(8)) #1

; Function Attrs: noinline uwtable
define internal void @_GLOBAL__sub_I_box.cpp() #0 section ".text.startup" {
entry:
  call void @__cxx_global_var_init()
  ret void
}

attributes #0 = { noinline uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nounwind }
attributes #4 = { mustprogress noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #5 = { mustprogress noinline norecurse optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 16.0.0 (https://github.com/llvm/llvm-project.git 3730658ed95cd82cc2942f39d55610a5d98f8b23)"}
