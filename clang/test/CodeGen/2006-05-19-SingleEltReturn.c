// Test returning a single element aggregate value containing a double.
// RUN: %clang_cc1 -triple i686-linux %s -emit-llvm -o - | FileCheck %s --check-prefix=X86_32
// RUN: %clang_cc1 %s -emit-llvm -o -

struct X {
  double D;
};

struct Y {
  struct X x;
};

struct Y bar();

void foo(struct Y *P) {
  *P = bar();
}

struct Y bar() {
  struct Y a;
  a.x.D = 0;
  return a;
}


// X86_32: define dso_local void @foo(%struct.Y* %P)
// X86_32:   call void @bar(%struct.Y* sret %{{[^),]*}})

// X86_32: define dso_local void @bar(%struct.Y* noalias sret %{{[^,)]*}})
// X86_32:   ret void
