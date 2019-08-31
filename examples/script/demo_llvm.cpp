// demo_llvm.cpp created on 2019-05-14, part of XCI toolkit
// Copyright 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "llvm/ADT/STLExtras.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetOptions.h"
#include <cassert>
#include <memory>
#include <vector>

using namespace llvm;

int main()
{
    InitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    LLVMContext context;
    std::unique_ptr<Module> owner = make_unique<Module>("test", context);
    Module* module = owner.get();

    Function *add1f =
        Function::Create(FunctionType::get(Type::getInt32Ty(context),
                                           {Type::getInt32Ty(context)}, false),
                         Function::ExternalLinkage, "add1", module);

    BasicBlock *bb = BasicBlock::Create(context, "EntryBlock", add1f);
    IRBuilder<> builder(bb);
    Value *one = builder.getInt32(11);

    assert(add1f->arg_begin() != add1f->arg_end()); // Make sure there's an arg
    Argument *argX = &*add1f->arg_begin();          // Get the arg
    argX->setName("AnArg");            // Give it a nice symbolic name for fun.

    Value *add = builder.CreateAdd(one, argX);
    builder.CreateRet(add);


    Function *foo_f =
            Function::Create(FunctionType::get(Type::getInt32Ty(context), {}, false),
                             Function::ExternalLinkage, "foo", module);
    bb = BasicBlock::Create(context, "EntryBlock", foo_f);
    builder.SetInsertPoint(bb);

    Value *ten = builder.getInt32(10);

    CallInst *add1CallRes = builder.CreateCall(add1f, ten);
    add1CallRes->setTailCall(true);

    builder.CreateRet(add1CallRes);

    outs() << "We just constructed this LLVM module:\n\n" << *module;

    if (verifyModule(*module)) {
        errs() << "Error: module failed verification.  This shouldn't happen.\n";
        abort();
    }

    outs() << "\n\nRunning foo: ";
    outs().flush();

    // Now we create the JIT.
    EngineBuilder engine_builder(std::move(owner));
    ExecutionEngine* ee = engine_builder.create();
    if (!ee) {
        errs() << "Error: execution engine creation failed.\n";
        return 1;
    }

    std::vector<GenericValue> noargs;
    GenericValue gv = ee->runFunction(foo_f, noargs);

    // Import result of execution:
    outs() << "Result: " << gv.IntVal << "\n";
    delete ee;
    llvm_shutdown();
    return 0;
}
