//===-- HelloWorld.h - Example Transformations ------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_UTILS_HELLOWORLD_H
#define LLVM_TRANSFORMS_UTILS_HELLOWORLD_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class HelloWorldPass : public PassInfoMixin<HelloWorldPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
  // PassInfoMaxin代替旧版的Apass::FunctionPass::Pass，避免了很多虚函数的override。现在只有APass.name()是继承而来的。

  // PreservedAnalyses保证所有的analyses结果在该pass后仍然有效
  // passmanager保证对module的所有func执行该run
  static bool isRequired() { return true; }
  // 不能被跳过的pass，例如AlwaysInlinePass
  // optnone：选择无，该优化会被跳过
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_HELLOWORLD_H
