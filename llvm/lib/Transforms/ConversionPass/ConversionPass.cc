#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/raw_ostream.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "llvm/Support/FileSystem.h"
#include <fstream>


#include <iostream>
#include <map>

#include <llvm/Support/Debug.h>

using namespace llvm;

// LLVM passes are normally defined in the anonymous namespace, as they should
// only ever be exposed via their superclass interface
namespace {

struct ConversionPass : ModulePass, InstVisitor<ConversionPass>
{
    /// The module that we're currently working on
    Module *M = 0;
    /// The data layout of the current module.
    DataLayout *DL = 0;
    /// Unique value.  Its address is used to identify this class.
    static char ID;
    /// Call the superclass constructor with the unique identifier as the
    /// (by-reference) argument.
    ConversionPass() : ModulePass(ID) {}

    struct PrefetchState
    {
        int currentInstructions=0;
        SmallVector<SetVector<Value*> , 8> Instructions;
        int currentAddrConfig = 0;
        Value* ArrayStart = nullptr;
        Instruction* ArrayEnd = nullptr;
        Instruction* ArraySize = nullptr;
        Type* ArrayType = nullptr;
        bool madeArrayEnd = false;
        IntrinsicInst* swprefetch = nullptr;
        BasicBlock* LoopParent = nullptr;
        BasicBlock* LoopEnd = nullptr;
        Function* f;
    };
    
    
    
void eraseKids(std::vector<Instruction*> instz, std::set<Instruction*> &instsToRemove) {
          
          for(auto &J : instz) {
					     if(!instsToRemove.count(J)) {
                            if(J->hasNUses(0)) {
                                if(J->getParent()) {
                                    dbgs() << "removing real " << (J) << "\n";
                                    Use* u2 = J->getOperandList();
									Use* end2 = u2 + J->getNumOperands();
									std::vector<Instruction*> insts;
									for(;u2<end2; u2++) {
										if(Instruction* K = dyn_cast<Instruction>(u2->get()))
										  insts.push_back(K);
									}
                                    J->dropAllReferences();
                                    instsToRemove.insert(J);
                                    eraseKids(insts,instsToRemove);
                                }
                            }
              
			}
		}
}

    struct pfround
    {
        SetVector<Value*> instructions;
        SmallVector<SetVector<Value*> , 4> representedInstructions;
        int currentAddrConfig = 0;
        Type* ArrayType = nullptr;
        bool isBaseRound = false;
        bool isBatch = false;
        int batchSize = 0;
        bool batchMerged = false;
        std::map<Value*,int> loadRounds;
    };
    
     SmallVector<Instruction* , 8> failedPFs;

    /// Return the name of the pass, for debugging.
    const char *getPassName() const override {
        return "Simple example pass";
    }
    
    void printAsFixedOperand(Value* i, raw_fd_ostream*  file) {
		                        std::string type_str;
                        llvm::raw_string_ostream s (type_str);
                        i->printAsOperand(s, false);
                        std::string st = s.str();
                        std::replace( st.begin(), st.end(), '@', '%');
                        
                        (*file) << st;
	}


    PHINode *getCanonicalishInductionVariable(Loop* L) const {
        BasicBlock *H = L->getHeader();

        BasicBlock *Incoming = nullptr, *Backedge = nullptr;
        pred_iterator PI = pred_begin(H);
        assert(PI != pred_end(H) &&
               "Loop must have at least one backedge!");
        Backedge = *PI++;
        if (PI == pred_end(H)) return nullptr;  // dead loop
        Incoming = *PI++;
        if (PI != pred_end(H)) return nullptr;  // multiple backedges?

        if (L->contains(Incoming)) {
            if (L->contains(Backedge))
                return nullptr;
            std::swap(Incoming, Backedge);
        } else if (!L->contains(Backedge))
            return nullptr;

        // Loop over all of the PHI nodes, looking for a canonical indvar.
        for (BasicBlock::iterator I = H->begin(); isa<PHINode>(I); ++I) {
            PHINode *PN = cast<PHINode>(I);
            //if (ConstantInt *CI =
            //  dyn_cast<ConstantInt>(PN->getIncomingValueForBlock(Incoming)))
            //if (CI->isNullValue())
            if (Instruction *Inc =
                        dyn_cast<Instruction>(PN->getIncomingValueForBlock(Backedge)))
                if (Inc->getOpcode() == Instruction::Add &&
                        Inc->getOperand(0) == PN)
                    if (ConstantInt *CI = dyn_cast<ConstantInt>(Inc->getOperand(1)))
                        if (CI->equalsInt(1))
                            return PN;
        }
        return nullptr;
    }


    Value *getCanonicalishSizeVariable(Loop* L) const {

        // Loop over all of the PHI nodes, looking for a canonical indvar.

        //Value* indVar = getCanonicalishInductionVariable(L);



        SmallVector<BasicBlock *, 4 > ExitingBlocks;

        L->getExitingBlocks(ExitingBlocks);

        Value * loopPtr = nullptr;

        for(auto &B : ExitingBlocks)
            for(Instruction &J :* B) {

                Instruction* I = &J;
                CmpInst *CI = dyn_cast<CmpInst>(I);

                //dbgs() << *I;
                //if (ConstantInt *CI =
                //  dyn_cast<ConstantInt>(PN->getIncomingValueForBlock(Incoming)))
                //if (CI->isNullValue())

                if(CI) {
                    if(L->isLoopInvariant(CI->getOperand(1))) loopPtr = CI->getOperand(1);

                    if(L->isLoopInvariant(CI->getOperand(0))) loopPtr =  CI->getOperand(0);

                    //printf("Size not loop invariant!\n");
                }



            }


        if(!loopPtr) printf("Size not loop invariant!\n");

        return loopPtr;
    }


    PHINode *getWeirdCanonicalishInductionVariable(Loop* L) const {
        BasicBlock *H = L->getHeader();

        BasicBlock *Incoming = nullptr, *Backedge = nullptr;
        pred_iterator PI = pred_begin(H);
        assert(PI != pred_end(H) &&
               "Loop must have at least one backedge!");
        Backedge = *PI++;
        if (PI == pred_end(H)) return nullptr;  // dead loop
        Incoming = *PI++;
        if (PI != pred_end(H)) return nullptr;  // multiple backedges?

        if (L->contains(Incoming)) {
            if (L->contains(Backedge))
                return nullptr;
            std::swap(Incoming, Backedge);
        } else if (!L->contains(Backedge))
            return nullptr;

        // Loop over all of the PHI nodes, looking for a canonical indvar.
        for (BasicBlock::iterator I = H->begin(); isa<PHINode>(I); ++I) {
            PHINode *PN = cast<PHINode>(I);
            //if (ConstantInt *CI =
            //  dyn_cast<ConstantInt>(PN->getIncomingValueForBlock(Incoming)))
            //if (CI->isNullValue())
            if (GetElementPtrInst *Inc =
                        dyn_cast<GetElementPtrInst>(PN->getIncomingValueForBlock(Backedge)))
                if (Inc->getOperand(0) == PN)
                    if (ConstantInt *CI = dyn_cast<ConstantInt>(Inc->getOperand(Inc->getNumOperands()-1)))
                        if (CI->equalsInt(1))
                            return PN;
        }
        return nullptr;
    }


    GetElementPtrInst *getWeirdCanonicalishInductionVariableGep(Loop* L) const {
        BasicBlock *H = L->getHeader();

        BasicBlock *Incoming = nullptr, *Backedge = nullptr;
        pred_iterator PI = pred_begin(H);
        assert(PI != pred_end(H) &&
               "Loop must have at least one backedge!");
        Backedge = *PI++;
        if (PI == pred_end(H)) return nullptr;  // dead loop
        Incoming = *PI++;
        if (PI != pred_end(H)) return nullptr;  // multiple backedges?

        if (L->contains(Incoming)) {
            if (L->contains(Backedge))
                return nullptr;
            std::swap(Incoming, Backedge);
        } else if (!L->contains(Backedge))
            return nullptr;

        // Loop over all of the PHI nodes, looking for a canonical indvar.
        for (BasicBlock::iterator I = H->begin(); isa<PHINode>(I); ++I) {
            PHINode *PN = cast<PHINode>(I);
            //if (ConstantInt *CI =
            //  dyn_cast<ConstantInt>(PN->getIncomingValueForBlock(Incoming)))
            //if (CI->isNullValue())
            if (GetElementPtrInst *Inc =
                        dyn_cast<GetElementPtrInst>(PN->getIncomingValueForBlock(Backedge)))
                if (Inc->getOperand(0) == PN)
                    if (ConstantInt *CI = dyn_cast<ConstantInt>(Inc->getOperand(Inc->getNumOperands()-1)))
                        if (CI->equalsInt(1))
                            return Inc;
        }
        return nullptr;
    }


    Value *getWeirdCanonicalishInductionVariableFirst(Loop* L) const {
        BasicBlock *H = L->getHeader();

        BasicBlock *Incoming = nullptr, *Backedge = nullptr;
        pred_iterator PI = pred_begin(H);
        assert(PI != pred_end(H) &&
               "Loop must have at least one backedge!");
        Backedge = *PI++;
        if (PI == pred_end(H)) return nullptr;  // dead loop
        Incoming = *PI++;
        if (PI != pred_end(H)) return nullptr;  // multiple backedges?

        if (L->contains(Incoming)) {
            if (L->contains(Backedge))
                return nullptr;
            std::swap(Incoming, Backedge);
        } else if (!L->contains(Backedge))
            return nullptr;

        // Loop over all of the PHI nodes, looking for a canonical indvar.
        for (BasicBlock::iterator I = H->begin(); isa<PHINode>(I); ++I) {
            PHINode *PN = cast<PHINode>(I);
            //if (ConstantInt *CI =
            //  dyn_cast<ConstantInt>(PN->getIncomingValueForBlock(Incoming)))
            //if (CI->isNullValue())
            if (GetElementPtrInst *Inc =
                        dyn_cast<GetElementPtrInst>(PN->getIncomingValueForBlock(Backedge)))
                if (Inc->getOperand(0) == PN)
                    if (ConstantInt *CI = dyn_cast<ConstantInt>(Inc->getOperand(Inc->getNumOperands()-1)))
                        if (CI->equalsInt(1))
                            return PN->getIncomingValueForBlock(Incoming);
        }
        return nullptr;
    }



    /// doInitialization - called when the pass manager begins running this
    /// pass on a module.  A single instance of the pass may be run on multiple
    /// modules in sequence.
    bool doInitialization(Module &Mod) override {
        M = &Mod;
        if (DL)
            delete DL;
        DL = new DataLayout(M);
        // Return false on success.

        return false;
    }

    /// doFinalization - called when the pass manager has finished running this
    /// pass on a module.  It is possible that the pass will be used again on
    /// another module, so reset it to its initial state.
    bool doFinalization(Module &Mod) override {
        assert(&Mod == M);




        delete DL;
        M = nullptr;
        DL = nullptr;
        // Return false on success.
        Values.clear();
        prefetchStates.clear();
        currentValue = 0;

        return false;
    }

    /// A vector where we'll collect the alloca instructions that we've visited.
    /// Note that, unlike std::vector<>, this takes a second template argument
    /// indicating the amount of space that will be allocated inside the object
    /// for storage.  If we have fewer than 16 objects, then this won't need to
    /// call `new` or `delete` to dynamically allocate memory.


    //std::map<int,int> blocksizes;

    SmallVector<PrefetchState,8> prefetchStates;

    ValueMap<Value*, int> Values;
    std::set<Type*> typeDefs;

    int currentValue = 0;

    virtual void getAnalysisUsage(AnalysisUsage& AU) const override {
        AU.addRequired<LoopInfoWrapperPass>();
    }


    void getTypeDefs(Type* t) {


        while(SequentialType* seqt = dyn_cast<SequentialType>(t)) {
            t = seqt->getElementType();
        }
        if(StructType* st = dyn_cast<StructType>(t)) {

            if(!typeDefs.count(st)) {
                typeDefs.insert(st);

                for(Type* nt : st->elements()) {

                    getTypeDefs(nt);
                }
            }



        }
    }


    int DFS (Value* v, Value* &NextLoad, Loop* L, int depth, bool& observed_iv, PrefetchState* p) {

        if(User* use  = dyn_cast<User>(v)) {
            Use* u = use->getOperandList();
            Use* end = u + use->getNumOperands();
            for(; u < end; u++) {
                for(int x=0; x<depth; x++) dbgs() << "-";
                u->get()->print(dbgs());
                dbgs() << "\n";


                if(dyn_cast<Constant>(u->get()) && !dyn_cast<GlobalValue>(u->get())) {
                    dbgs() << "Got constant!\n";
                } else {
                    if(p->Instructions[p->currentInstructions].count(u->get()))p->Instructions[p->currentInstructions].remove(u->get()); //to move instruction earlier.
                    p->Instructions[p->currentInstructions].insert(u->get());
                    if(L->isLoopInvariant(u->get())) {
                        dbgs() << "Got loop invariant!\n";
                        if(!Values.count(u->get())) {

                            Type* t = u->get()->getType();

                            getTypeDefs(t);

                            Values.insert(std::pair<Value*,int>(u->get(),currentValue++));

                        }

                    }
                    else if(dyn_cast<LoadInst>(u->get())) {
                        if(!NextLoad) {
                            NextLoad = u->get();
                            dbgs() << "\nLoad found!";
                            NextLoad->print(dbgs());
                            dbgs()<<"\n";

                            if(observed_iv) {
                                dbgs() << "IV and Load at once: error!\n";
                                return -1;
                            }
                        }
                        else if (NextLoad != u->get()) {
                            dbgs() << "Two loads: fail!";
                            u->get()->print(dbgs());
                            dbgs() << "versus";
                            NextLoad->print(dbgs());
                            NextLoad = NULL;
                            dbgs() << "\n";
                            return -1;
                        }
                    }
                    else if (dyn_cast<PHINode>(u->get())) {
                        if(dyn_cast<PHINode>(u->get()) == getCanonicalishInductionVariable (L) || dyn_cast<PHINode>(u->get()) == getWeirdCanonicalishInductionVariable(L)) {
                            dbgs() << "Observed Induction Variable!\n";
                            observed_iv=true;
                            if(NextLoad) {
                                dbgs() << "IV and Load at once: error!\n";
                                return -1;
                            }
                        }
                        else {
                            dbgs() << "Non-induction Phi node: fail!\n"; //TODO: Overly conservative.
                            return -1;
                        }
                    }
                    else if(DFS(u->get(), NextLoad, L, depth+1, observed_iv, p)) return -1;
                }
            }
        }

        return 0;
    }

    /// Example visit method.  This is called once for each alloca instruction
    /// in the function.  Implement methods like this to inspect different
    /// instructions.
    /* void visitAllocaInst(AllocaInst &AI) {
       // Log the alloca to the standard error
       AI.dump();
       visitedAllocas.push_back(&AI);
     }*/

    bool runOnModule(Module &Mod) override {


        //visitedAllocas.clear();
        // The visit method is inherited by InstVisitor.  This will call each
        // of the visit*() methods, allowing individual functions to be inspected.
        //visit(F);
        //llvm::SmallVector<AllocaInst*, 16> foundAllocas;
        // Alternatively, we can loop over each basic block and then over each
        // instruction and inspect them individually:




        Constant* c = Mod.getOrInsertFunction("set_addr_bounds",
                                              /*ret type*/                           Type::getVoidTy (Mod.getContext()),
                                              /*args*/                               Type::getInt64Ty (Mod.getContext()),
                                              Type::getInt64Ty (Mod.getContext()),
                                              Type::getInt64Ty (Mod.getContext()),
                                              Type::getInt64Ty (Mod.getContext()),
                                              /*varargs terminated with null*/       NULL);

        Function* fun = cast<Function>(c);
        fun->setCallingConv(CallingConv::C);


        //Constant* check = Mod.getOrInsertFunction("m5_checkpoint",
        //                                     /*ret type*/                           Type::getVoidTy (Mod.getContext()),
        //                                   /*args*/                               Type::getInt64Ty (Mod.getContext()),
        //                                 Type::getInt64Ty (Mod.getContext()),
        //                               /*varargs terminated with null*/       NULL);

        // Function* checkfun = cast<Function>(check);
        // checkfun->setCallingConv(CallingConv::C);



        bool modified = false;


        for (auto &F : Mod) {

            if(F.isDeclaration()) continue;
            LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();

            for (auto &BB : F) {





                for (auto &I : BB) {


                    if (IntrinsicInst *i = dyn_cast<IntrinsicInst>(&I)) {

                        if(Intrinsic::getName(i->getIntrinsicID()) == "llvm.prefetch") {



                            PrefetchState pfs;
                            prefetchStates.push_back(pfs);
                            PrefetchState* p = &prefetchStates.back();
                            p->f = &F;

                            p->swprefetch = i;
                            int oldValues = currentValue;
                            dbgs() << "Got software prefetch:\n" ;


                            (i->getOperandUse(0).get())->print(dbgs());

                            dbgs() << "\n" ;


                            p->Instructions.resize(p->currentInstructions+1);
                            p->Instructions[p->currentInstructions].insert(i);
                            p->Instructions[p->currentInstructions].insert(i->getOperandUse(0).get());

                            bool error = false;


                            Loop* L = LI.getLoopFor(&BB);

                            if(L) {
                                p->LoopParent = L->getLoopPredecessor();
                                p->LoopEnd = L->getUniqueExitBlock ();




                                auto weirdInd = getWeirdCanonicalishInductionVariable(L);

                                auto induction = getCanonicalishInductionVariable (L);

                                if(induction || weirdInd) {

                                    bool observed_iv = false;

                                    Value* NextLoad = NULL;

                                    Value* oldLoad = i->getOperandUse(0).get();

                                    do {


                                        dbgs() << "-----------\n";

                                        p->Instructions[p->currentInstructions].insert(oldLoad);

                                        observed_iv=false;
                                        NextLoad = NULL;
                                        if(DFS(oldLoad, NextLoad, L, 0, observed_iv, p)) {
                                            error =true;
                                            break;
                                        }
                                        if(observed_iv) {
                                            dbgs () << "Load bounds in ";

                                            GetElementPtrInst* gep;

                                            if(weirdInd) {
                                                oldLoad = getWeirdCanonicalishInductionVariableFirst(L);
                                                p->ArrayStart = oldLoad;
                                                p->ArrayStart->print(dbgs());

                                                dbgs() << "No size info: fall back on size of loop!\n";

                                                Value* size = getCanonicalishSizeVariable(L);


                                                if(!size) {
                                                    dbgs() << "Error: can't find size!\n";
                                                    error = true;
                                                } else {
                                                    dbgs() << "Got size from: ";
                                                    size->print(dbgs());
                                                    dbgs() << "\n";



                                                    p->ArrayEnd = dyn_cast<GetElementPtrInst>(size);

                                                    assert(p->ArrayEnd);



                                                    dbgs() << " so GEP is " << *(p->ArrayEnd)<< "\n";


                                                    p->ArrayType = dyn_cast<GetElementPtrInst>(size)->getSourceElementType ();
                                                }


                                            } else {

                                                if(LoadInst* l = dyn_cast<LoadInst>(oldLoad)) {
                                                    l->getPointerOperand()->print(dbgs());

                                                    gep = dyn_cast<GetElementPtrInst>(l->getPointerOperand());
                                                } else {
                                                    CastInst* cast = dyn_cast<CastInst>(i->getOperand(0));
                                                    assert(cast);

                                                    cast->getOperand(0)->print(dbgs());

                                                    gep = dyn_cast<GetElementPtrInst>(cast->getOperand(0));
                                                }

                                                dbgs() << " with ptr: ";
                                                gep->getPointerOperand()->print(dbgs());


                                                p->ArrayStart = gep->getPointerOperand();

                                                if(ArrayType* at = dyn_cast<ArrayType>(gep->getSourceElementType ())) {
                                                    dbgs() << " and size: ";
                                                    dbgs() << std::to_string(at->getNumElements());
                                                    int size = at->getNumElements();
                                                    dbgs() << "and type: " << *(at->getElementType ());
                                                    p->ArrayType = at->getElementType ();

                                                    Value* ar[] = {
                                                        ConstantInt::get(Type::getInt64Ty(Mod.getContext()),0),
                                                        ConstantInt::get(Type::getInt64Ty(Mod.getContext()),size)
                                                    };
                                                    p->ArrayEnd = GetElementPtrInst::Create(gep->getSourceElementType (), p->ArrayStart, ar);
                                                    p->madeArrayEnd = true;
                                                } else if(AllocaInst* ai = dyn_cast<AllocaInst>(p->ArrayStart)) {
                                                    dbgs() << "and dynamic allocated size " << *(ai->getArraySize());

                                                    //dbgs() << *(gep->getSourceElementType ());
                                                    //SequentialType* pt = dyn_cast<SequentialType>(gep->getSourceElementType ());
                                                    dbgs() << "and type: ";
                                                    dbgs() << *(gep->getSourceElementType ());//*(pt->getElementType ());
                                                    p->ArrayType = (gep->getSourceElementType ());


                                                    Value* ar[] = {
                                                        //  ConstantInt::get(Type::getInt64Ty(Mod.getContext()),0),
                                                        // ConstantInt::get(Type::getInt64Ty(Mod.getContext()),0),
                                                        ai->getArraySize ()
                                                    };

                                                    p->ArrayEnd = GetElementPtrInst::Create(gep->getSourceElementType (), p->ArrayStart, ar);
                                                    p->madeArrayEnd = true;



                                                } /*else if(Argument* arg =  dyn_cast<Argument>(p->ArrayStart))

                                            {
                                                dbgs() << "from argument: not yet implemented!" << *arg << "\n";
                                                error = true;

                                            }*/ else {
                                                    dbgs() << "No size info: fall back on size of loop!\n";

                                                    Value* size = getCanonicalishSizeVariable(L);


                                                    if(!size) {
                                                        dbgs() << "Error: can't find size!\n";
                                                        error = true;
                                                    } else {
                                                        dbgs() << "Got size from: ";
                                                        size->print(dbgs());
                                                        dbgs() << "\n";


                                                        if(Instruction* I = dyn_cast<Instruction>(size)) {
                                                            p->ArraySize = I;
                                                        }

                                                        /*  Value* ar[] = {

                                                        size,
                                                        ConstantInt::get(Type::getInt32Ty(Mod.getContext()),0)
                                                        };*/

                                                        std::vector<Value*> ar(gep->getNumIndices());



                                                        int z=0;
                                                        for(Use* v = gep->idx_begin(); v!= gep->idx_end(); v++, z++) {  //TODO: potentially a bit hacky!
                                                            if(!L->isLoopInvariant(v->get())) {

                                                                ar[z] = size;
                                                            } else ar[z] = v->get();
                                                        }

                                                        ArrayRef<Value*> aa(ar);

                                                        p->ArrayEnd = GetElementPtrInst::Create(gep->getSourceElementType (), p->ArrayStart, aa);
                                                        p->madeArrayEnd = true;

                                                        dbgs() << " so GEP is " << *(p->ArrayEnd)<< "\n";

                                                        dbgs() << "ptr type: " << *(gep->getSourceElementType ()) << "\n";

                                                        p->ArrayType = gep->getSourceElementType ();


                                                        //TODO: is this general enough? Will it always be a ptr? Probably...
                                                    }




                                                }

                                                Loop * lp = LI.getLoopFor(p->LoopParent); //Move config outwards.
     

                                                dbgs()<<"\n";
                                                
                                                /*while (lp && lp->getUniqueExitBlock () &&  lp->getLoopPredecessor() &&  p->ArrayStart && lp->isLoopInvariant(p->ArrayStart) && (!p->ArraySize || lp->isLoopInvariant(p->ArraySize))) {
                                                    p->LoopParent = lp->getLoopPredecessor();
                                                    p->LoopEnd = lp->getUniqueExitBlock ();

                                                    lp = LI.getLoopFor(p->LoopParent);
                                                    dbgs()  << "pushed to outer loop\n";
                                                }*/

                                            }

                                        }

                                        //Instruction* arrSt = p->ArrayStart;

                                        //if(CastInst* ci = dyn_cast<CastInst>(p->ArrayStart)) {
                                        //arrst = ci->getOperand(0);
                                        //}




                                        p->currentInstructions++;
                                        p->Instructions.resize(p->currentInstructions+1);

                                        oldLoad = NextLoad;

                                    } while (NextLoad != NULL && !error);

                                } else {
                                    dbgs()<< "Error: couldn't find loop induction variable!\n";
                                    error = true;
                                }


                            } else {
                                dbgs()<< "Error: couldn't find loop!\n";
                                error = true;
                            }


                            if(error) {

                                if(p->ArrayEnd && p->madeArrayEnd) {
                                    p->ArrayEnd->dropAllReferences();
                                    p->ArrayEnd->eraseFromParent();
                                }
                                
                                failedPFs.push_back(p->swprefetch);
                                
                                prefetchStates.pop_back();
                                
                                

                                SmallVector<Value*,8> removeList;




                                for(auto i : Values) {
                                    if(i->second > oldValues) {
                                        //add to remove list
                                        removeList.push_back(i->first);
                                    }
                                }

                                for(auto i: removeList) {
                                    Values.erase(i);
                                }
                            }
                        }

                    }

                }





            }
        }


        // Note that we *must not* modify the IR in either of the previous methods,
        // because doing so will invalidate the iterators that we're using to
        // explore it.  After we've finished using the iterators, it is safe to do
        // the modifications.
        //assert(foundAllocas.size() == visitedAllocas.size());
        // We have not modified this function.

        // Instruction* firstInsert = nullptr;
        ValueMap<Value*, int> ArrayValues;


        for (auto &p : prefetchStates) {
            assert(p.ArrayStart != nullptr);





            if(!ArrayValues.count(p.LoopParent->getTerminator())) { //This is so that we can use a given trigger for a particular loop.

                BasicBlock* BB = nullptr;

                /*if(dyn_cast<Instruction>(p.ArrayStart)) {
                    BB = dyn_cast<Instruction>(p.ArrayStart)->getParent();

                } else if(dyn_cast<Argument>(p.ArrayStart)) {
                    BB = &(dyn_cast<Argument>(p.ArrayStart)->getParent()->front());
                } else if(dyn_cast<Constant>(p.ArrayStart)) {

                    BB = p.LoopParent;
                }*/


                //if(p.ArraySize) BB = p.ArraySize->getParent();

                BB = p.LoopParent;

                assert(BB != nullptr);
                Instruction* in = BB->getTerminator();




                CastInst* c = CastInst::CreatePointerCast(p.ArrayStart,Type::getInt64Ty(Mod.getContext()));




                //BB->getInstList().insert(in,c);
                c->insertBefore(in);

                dbgs() << (*c);

                // if(!firstInsert)firstInsert = c;

                if(p.madeArrayEnd) p.ArrayEnd->insertBefore(in);//BB->getInstList().insert(in,p.ArrayEnd);

                dbgs() << (*(p.ArrayEnd));

                CastInst* d = CastInst::CreatePointerCast(p.ArrayEnd,Type::getInt64Ty(Mod.getContext()));

                //BB->getInstList().insert(in,d);
                d->insertBefore(in);
                //d->insertAfter(p.ArrayEnd);

                dbgs() << (*d);

                int id = currentValue++;

                p.currentAddrConfig = id;

                dbgs() << "Address Range Config:" << std::to_string(id) << " ";


                Value* ar[] = {
                    ConstantInt::get(Type::getInt64Ty(Mod.getContext()),id),
                    c,
                    d,
                    ConstantInt::get(Type::getInt64Ty(Mod.getContext()),0)
                };


                CallInst* e = CallInst::Create(fun,ar);
                //BB->getInstList().insert(in,e);
                e->insertAfter(d);

                if(p.LoopEnd) {

                    Value* ar2[] = {
                        ConstantInt::get(Type::getInt64Ty(Mod.getContext()),id),
                        ConstantInt::get(Type::getInt64Ty(Mod.getContext()),0),
                        ConstantInt::get(Type::getInt64Ty(Mod.getContext()),0),
                        ConstantInt::get(Type::getInt64Ty(Mod.getContext()),0)
                    };

                    CallInst* e2 = CallInst::Create(fun,ar2);
                    //p.LoopEnd->getInstList().insert(*(p.LoopEnd->getFirstInsertionPt ()),e2);
                    e2->insertBefore(&(*(p.LoopEnd->getFirstInsertionPt ())));
                }


                dbgs() << (*e);
                dbgs() << "\n";

                ArrayValues.insert(std::pair<Value*,int>(p.LoopParent->getTerminator(),id));



                modified = true;

            } else {
                p.ArrayEnd->dropAllReferences();
                //p.ArrayEnd->eraseFromParent();

                p.currentAddrConfig = ArrayValues.lookup(p.LoopParent->getTerminator());


            }


        }

        /*  if(firstInsert) {
        	    Value* ar[] = {
                  ConstantInt::get(Type::getInt64Ty(Mod.getContext()),0),
                  ConstantInt::get(Type::getInt64Ty(Mod.getContext()),0)
              };


              CallInst* e = CallInst::Create(checkfun,ar);
              (firstInsert->getParent()->getParent()->getEntryBlock()).getInstList().insert(firstInsert->getParent()->getParent()->getEntryBlock().getTerminator(),e);
        }*/

        std::set<Value*> valuez;

        for (auto &p : prefetchStates) {
            for(int x=p.currentInstructions-1; x>=0; x--) {
                for(auto i = p.Instructions[x].end()-1; i != p.Instructions[x].begin()-1; --i) {
                    if(Values.count(*i) && (!valuez.count(*i) || dyn_cast<Constant>(*i))) {
                        valuez.insert(*i);
                        BasicBlock* BB = nullptr;

                        if(dyn_cast<Instruction>(*i)) {
                            BB = dyn_cast<Instruction>(*i)->getParent();

                        } else if(dyn_cast<Argument>(*i)) {
                            BB = &(dyn_cast<Argument>(*i)->getParent()->front());
                        } else if(dyn_cast<Constant>(*i)) {

                            BB = p.LoopParent;
                        }

                        assert(BB != nullptr);

                        assert(BB->getTerminator() != nullptr);

                        Instruction* in = BB->getTerminator();


                        Type* t = ((*i)->getType());
                        if(t->isIntegerTy (64)) {

                            Value* ar[] = {
                                ConstantInt::get(Type::getInt64Ty(Mod.getContext()),Values.lookup(*i)),
                                ConstantInt::get(Type::getInt64Ty(Mod.getContext()),0),
                                ConstantInt::get(Type::getInt64Ty(Mod.getContext()),0),
                                *i
                            };


                            CallInst* d = CallInst::Create(fun,ar);
                            //BB->getInstList().insert(in,d);
                            d->insertBefore(in);

                            dbgs() << *d;
                        } else if (t->isIntegerTy()) {

                            CastInst* c = CastInst::CreateIntegerCast(*i,Type::getInt64Ty(Mod.getContext()),true); //true? isSigned?


                           // BB->getInstList().insert(in,c);
							c->insertBefore(in);


                            dbgs() << *c;

                            Value* ar[] = {
                                ConstantInt::get(Type::getInt64Ty(Mod.getContext()),Values.lookup(*i)),
                                ConstantInt::get(Type::getInt64Ty(Mod.getContext()),0),
                                ConstantInt::get(Type::getInt64Ty(Mod.getContext()),0),
                                c
                            };


                            CallInst* d = CallInst::Create(fun,ar);
                            //BB->getInstList().insert(in,d);
                            d->insertBefore(in);

                            dbgs() << *d;
                        }


                        else {
                            CastInst* c = CastInst::CreatePointerCast(*i,Type::getInt64Ty(Mod.getContext()));

							c->insertBefore(in);
                            //BB->getInstList().insert(in,c);



                            dbgs() << *c;

                            Value* ar[] = {
                                ConstantInt::get(Type::getInt64Ty(Mod.getContext()),Values.lookup(*i)),
                                ConstantInt::get(Type::getInt64Ty(Mod.getContext()),0),
                                ConstantInt::get(Type::getInt64Ty(Mod.getContext()),0),
                                c
                            };


                            CallInst* d = CallInst::Create(fun,ar);
                            //BB->getInstList().insert(in,d);
                            d->insertBefore(in);

                            dbgs() << *d;
                        }


                        //c->eraseFromParent();
                        //d->eraseFromParent();


                        modified = true;
                        //c->setTailCall();
                    }
                }
            }
        }


        int round = 0;

        std::vector<pfround> rounds;


        for(auto &p : prefetchStates) {

            bool foundLastRound=false;
            int lastFound = -1;


            for(int x=p.currentInstructions-1; x>=0; x--) { // round++

                bool found = false;
                for(auto z=0ul; z<rounds.size(); z++) {
                    for(auto &insts : rounds[z].representedInstructions) {

                        bool same = true;

                        if(insts.size() != p.Instructions[x].size()) same = false;

                        if(same) {
                            for(unsigned long q=1; q<insts.size(); q++) { //off by one, to avoid prefetches that aren't technically the same.
                                if(insts[q] != (p.Instructions[x])[q]) {
                                    same = false;
                                    break;
                                }
                            }
                        }

                        if(same) {
                            found = true;
                            foundLastRound=true;
                            lastFound = rounds[z].loadRounds.find(insts[0])->second;
                            break;
                        } //found a copy

                    }
                    if(found) break;
                }

                if(!found) {

                    if(x==p.currentInstructions-1) { //for merging when the trigger is the same.
                        for(auto z=0ul; z<rounds.size(); z++) {
                            if(rounds[z].isBaseRound && rounds[z].currentAddrConfig == p.currentAddrConfig) {
                                foundLastRound = true;
                                lastFound = z;
                            }
                        }
                    }

                    if(!foundLastRound) {
                        pfround newround;
                        rounds.push_back(newround);
                        pfround* nuround = &rounds.back();
                        nuround->currentAddrConfig = p.currentAddrConfig;
                        nuround->ArrayType = p.ArrayType;
                        if(x==p.currentInstructions-1) nuround->isBaseRound = true;
                        nuround->instructions = p.Instructions[x];
                        nuround->representedInstructions.push_back(p.Instructions[x]);

                        if(LoadInst* li = dyn_cast<LoadInst>(*(p.Instructions[x].begin()))) {
                            rounds[round].loadRounds.insert(std::pair<Value*,int>(li,round+1));
                            dbgs() << "round " << std::to_string(round) << " to " << std::to_string(round+1) << " " << *li << "\n";
                        } else dbgs() << "round " << std::to_string(round) << "noptr\n";
                        round++;

                    } else {
                        dbgs() << "got " << std::to_string(lastFound)  << "\n";

                        for(auto q = rounds[lastFound].instructions.end()-1; q> rounds[lastFound].instructions.begin()-1; q--) dbgs() <<( **q) << "\n";



                        dbgs() << "vs \n";

                        for(auto q = p.Instructions[x].end()-1; q> p.Instructions[x].begin()-1; q--) dbgs() <<( **q) << "\n";

                        dbgs() << "\n";

                        //dbgs() << "got " << rounds[lastFound].instructions.getArrayRef() << " vs " << p.Instructions[x].getArrayRef() "\n";

                        rounds[lastFound].representedInstructions.push_back(p.Instructions[x]);

                        SetVector<Value*> oldInsts = SetVector<Value*>();
                        for(auto q = rounds[lastFound].instructions.end()-1; q> rounds[lastFound].instructions.begin()-1; q--)
                            oldInsts.insert(*q);

                        for(auto q = p.Instructions[x].end()-1; q> p.Instructions[x].begin()-1; q--) oldInsts.insert(*q);
                        SetVector<Value*> newInsts = SetVector<Value*>();
                        for(auto q = oldInsts.end()-1; q > oldInsts.begin()-1; q--) newInsts.insert(*q);
                        rounds[lastFound].instructions = newInsts;
                        foundLastRound = false;
                        if(LoadInst* li = dyn_cast<LoadInst>(*(p.Instructions[x].begin()))) {
                            rounds[lastFound].loadRounds.insert(std::pair<Value*,int>(li,round));
                            dbgs() << "round " << std::to_string(lastFound) << " to " << std::to_string(round) << " " << *li << "\n";
                        }
                    }

                }
            }

        }

        SmallSet <int, 8> specials;
        std::map<int,int> functionMap;

        //std::ofstream myfile;
        //myfile.open ("pfoutput.ll");

        std::error_code EC;
        raw_fd_ostream myfile ("pfoutput.ll", EC,sys::fs::F_None);

        if(prefetchStates.size() !=0) {
            myfile << "; ModuleID = 'pfoutput.c'\ntarget datalayout = \"e-m:e-i64:64-i128:128-n32:64-S128\"\ntarget triple = \"aarch64-arm-none-eabi\"\n\n\n";
        }

        for(auto t = typeDefs.begin(); t != typeDefs.end(); t++) {
            dbgs() << **t << "\n";
            myfile << **t << "\n";
        }

        myfile << "\n\n";
        dbgs()<< "\n\n";

        round=0;

        for(auto &r: rounds) {

            if(r.isBaseRound && r.loadRounds.size() == 1 && rounds[round+1].loadRounds.size() == 0) {
                r.isBatch = true;
                rounds[round+1].batchMerged = true;
            } else if(r.isBaseRound && r.loadRounds.size() == 0) {
                r.isBatch = true;
            }

            if(r.batchMerged) {
                for(auto &i: r.instructions) {
                    if(!r.loadRounds.count(i) && dyn_cast<LoadInst>(i)) {
                        Type* h = ((i)->getType());
                        Type* t = r.ArrayType;
                        if(t->isIntegerTy(64) || h->isPointerTy()) {
                            r.batchSize = 8;
                        } else if(t->isIntegerTy(32)) {
                            r.batchSize = 16;
                        } else {
                            r.batchMerged = false;
                            rounds[round-1].isBatch = false;
                        }
                    }
                }
            }

            round++;
        }

        round=0;


        for (auto &p : rounds) {

            dbgs() << "----------------------------\n Instructions for round " << std::to_string(round) << ":\n";
            myfile << "define void @round" << std::to_string(round) << "() #0 {\n";

            if(p.batchMerged) {
                dbgs() << "entry: \n";
                myfile << "entry: \n";
            }

            for(auto i = p.instructions.end()-1; i != p.instructions.begin()-1; --i) {
                if(Values.count(*i)) {
                    //dbgs() << " CONFIG " << std::to_string(Values.lookup(*i)) << " : ";
                    Type* t = ((*i)->getType());
                    if(t->isIntegerTy (64))
                    {
                        (*i)->printAsOperand(dbgs(), false);
                        dbgs() << " = call i64 @get_state_data(i64 " << std::to_string(14 + 4*Values.lookup(*i)) <<")\n" ;
                        printAsFixedOperand((*i), &myfile);
                        myfile << " = call i64 @get_state_data(i64 " << std::to_string(14 + 4*Values.lookup(*i)) <<")\n" ;
                    } else {
                        (*i)->printAsOperand(dbgs(), false);
                        dbgs() << "temp = call i64 @get_state_data(i64 " << std::to_string(14 + 4*Values.lookup(*i)) <<")\n" ;
						printAsFixedOperand((*i), &myfile);
                        myfile  << "temp = call i64 @get_state_data(i64 " << std::to_string(14 + 4*Values.lookup(*i)) <<")\n" ;

                        if(t->isIntegerTy()) {
                            (*i)->printAsOperand(dbgs(), false);
                            dbgs() << " = trunc i64 ";
                            (*i)->printAsOperand(dbgs(), false);
                            dbgs() << "temp to ";
                            (*i)->getType()->print(dbgs());
                            dbgs() << "\n";

                            printAsFixedOperand((*i), &myfile);
                            myfile << " = trunc i64 ";
                            printAsFixedOperand((*i), &myfile);
                            myfile << "temp to ";
                            (*i)->getType()->print(myfile);
                            myfile << "\n";

                        } else if (t->isPointerTy()) {
                            (*i)->printAsOperand(dbgs(), false);
                            dbgs() << " = inttoptr i64 ";
                            (*i)->printAsOperand(dbgs(), false);
                            dbgs() << "temp to ";
                            (*i)->getType()->print(dbgs());
                            dbgs() << "\n";

                            printAsFixedOperand((*i), &myfile);
                            myfile << " = inttoptr i64 ";
                            printAsFixedOperand((*i), &myfile);
                            myfile << "temp to ";
                            (*i)->getType()->print(myfile);
                            myfile << "\n";
                        } else if(t->isFloatingPointTy()) {
                            if(t->isDoubleTy ()) {
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << " = bitcast i64 ";
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << "temp to ";
                                (*i)->getType()->print(dbgs());
                                dbgs() << "\n";

                                printAsFixedOperand((*i), &myfile);
                                myfile << " = bitcast i64 ";
                                printAsFixedOperand((*i), &myfile);
                                myfile << "temp to ";
                                (*i)->getType()->print(myfile);
                                myfile << "\n";
                            } else {
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << "temp2 = bitcast i64 ";
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << "temp to fp64\n";

                                dbgs() << " = bitcast fp64 ";
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << "temp2 to ";
                                (*i)->getType()->print(dbgs());
                                dbgs() << "\n";



                                printAsFixedOperand((*i), &myfile);
                                myfile << "temp2 = bitcast i64 ";
                                printAsFixedOperand((*i), &myfile);
                                myfile << "temp to fp64\n";

                                myfile << " = bitcast fp64 ";
                                printAsFixedOperand((*i), &myfile);
                                myfile << "temp2 to ";
                                (*i)->getType()->print(myfile);
                                myfile << "\n";
                            }


                        }

                    }
                }

            }

            if(p.batchMerged) {

                dbgs() <<  " br label %loop\n\n";
                dbgs() << "loop:       ; preds = %loop, %entry\n";


                dbgs() << "%i = phi i64 [0, %entry ], [ %nextvar, %loop ]\n";

                dbgs() << "%nextvar = add i64 %i, 1\n";


                myfile <<  " br label %loop\n\n";
                myfile << "loop:       ; preds = %loop, %entry\n";


                myfile << "%i = phi i64 [0, %entry ], [ %nextvar, %loop ]\n";

                myfile << "%nextvar = add i64 %i, 1\n";



            }

            for(auto i = p.instructions.end()-1; i != p.instructions.begin()-1; --i) {
                if(Values.count(*i)) {

                }
                else if (!p.loadRounds.count(*i) && dyn_cast<LoadInst>(*i)) {
                    specials.insert(round);
                    Type* t = ((*i)->getType());


                    if(t->isIntegerTy (64))
                    {
                        (*i)->printAsOperand(dbgs(), false);
                        dbgs() << " = call i64 @get_state_data(i64 " << (p.batchMerged ? "%i" : std::to_string(8)) <<")\n" ;
                        printAsFixedOperand((*i), &myfile);
                        myfile << " = call i64 @get_state_data(i64 " << (p.batchMerged ? "%i" : std::to_string(8))  <<")\n" ;
                    } else {

                        if(t->isIntegerTy(32) && p.batchMerged) {

                            dbgs() << "%offe = urem i64 %i, 2\n";

                            dbgs() << "%off = shl i64 %offe, 5\n";

                            dbgs() << "%ind = lshr i64 %i, 1\n";

                            dbgs() << "%vall = call i64 @get_state_data(i64 %ind)\n" ;

                            (*i)->printAsOperand(dbgs(), false);

                            dbgs() << "temp = lshr i64 %vall, %off\n";


                            myfile<< "%offe = urem i64 %i, 2\n";

                            myfile << "%off = shl i64 %offe, 5\n";

                            myfile << "%ind = lshr i64 %i, 1\n";

                            myfile << "%vall = call i64 @get_state_data(i64 %ind)\n" ;

                            printAsFixedOperand((*i), &myfile);

                            myfile << "temp = lshr i64 %vall, %off\n";

                        } else {


                            (*i)->printAsOperand(dbgs(), false);
                            dbgs() << "temp = call i64 @get_state_data(i64 " << (p.batchMerged ? "%i" : std::to_string(8))  <<")\n" ;
                        std::string type_str;
                        llvm::raw_string_ostream s (type_str);
                        (*i)->printAsOperand(s, false);
                        std::string st = s.str();
                        std::replace( st.begin(), st.end(), '@', '%');
                        myfile << st <<  "temp = call i64 @get_state_data(i64 " << (p.batchMerged ? "%i" : std::to_string(8))  <<")\n" ;

                        }

                        if(t->isIntegerTy()) {
                            (*i)->printAsOperand(dbgs(), false);
                            dbgs() << " = trunc i64 ";
                            (*i)->printAsOperand(dbgs(), false);
                            dbgs() << "temp to ";
                            (*i)->getType()->print(dbgs());
                            dbgs() << "\n";

                            printAsFixedOperand((*i), &myfile);
                            myfile << " = trunc i64 ";
                            printAsFixedOperand((*i), &myfile);
                            myfile << "temp to ";
                            (*i)->getType()->print(myfile);
                            myfile << "\n";

                        } else if (t->isPointerTy()) {
                            (*i)->printAsOperand(dbgs(), false);
                            dbgs() << " = inttoptr i64 ";
                            (*i)->printAsOperand(dbgs(), false);
                            dbgs() << "temp to ";
                            (*i)->getType()->print(dbgs());
                            dbgs() << "\n";

                            printAsFixedOperand((*i), &myfile);
                            myfile << " = inttoptr i64 ";
                            printAsFixedOperand((*i), &myfile);
                            myfile << "temp to ";
                            (*i)->getType()->print(myfile);
                            myfile << "\n";

                        } else if(t->isFloatingPointTy()) {
                            if(t->isDoubleTy ()) {
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << " = bitcast i64 ";
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << "temp to ";
                                (*i)->getType()->print(dbgs());
                                dbgs() << "\n";

                                printAsFixedOperand((*i), &myfile);
                                myfile << " = bitcast i64 ";
                                printAsFixedOperand((*i), &myfile);
                                myfile << "temp to ";
                                (*i)->getType()->print(myfile);
                                myfile << "\n";

                            } else {
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << "temp2 = bitcast i64 ";
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << "temp to fp64\n";

                                dbgs() << " = bitcast fp64 ";
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << "temp2 to ";
                                (*i)->getType()->print(dbgs());
                                dbgs() << "\n";


                                printAsFixedOperand((*i), &myfile);
                                myfile << "temp2 = bitcast i64 ";
                                printAsFixedOperand((*i), &myfile);
                                myfile << "temp to fp64\n";

                                myfile << " = bitcast fp64 ";
                                printAsFixedOperand((*i), &myfile);
                                myfile << "temp2 to ";
                                (*i)->getType()->print(myfile);
                                myfile << "\n";
                            }


                        }

                    }
                }
                else if (dyn_cast<PHINode>(*i)) {

                    functionMap.insert(std::pair<int,int>(round,p.currentAddrConfig));

                    Type* g = ((*i)->getType());

                    if(g->isIntegerTy()) {

                        dbgs() << "%addr = call i64 @get_state_data(i64 9)\n";
                        dbgs() << "%base = call i64 @get_state_data(i64 " << std::to_string(12 + 4*p.currentAddrConfig) << ")\n";
                        (*i)->printAsOperand(dbgs(), false);
                        dbgs() << "subt = sub i64 %addr, %base\n";

                        myfile << "%addr = call i64 @get_state_data(i64 9)\n";
                        myfile << "%base = call i64 @get_state_data(i64 " << std::to_string(12 + 4*p.currentAddrConfig) << ")\n";
                        (*i)->printAsOperand( myfile , false);
                        myfile  << "subt = sub i64 %addr, %base\n";




                        Type* t = p.ArrayType;
                        if(t->isIntegerTy() || t->isFloatTy() || t->isDoubleTy()) {

                            bool is64 = (*i)->getType()->isIntegerTy(64);
                            if(t->isIntegerTy(64) || t->isDoubleTy()) {
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << (is64 ? "": "temp") << " = ashr i64 ";
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() <<  "subt , 3\n";

                                printAsFixedOperand((*i), &myfile);
                                myfile << (is64 ? "": "temp") << " = ashr i64 ";
                                printAsFixedOperand((*i), &myfile);
                                myfile <<  "subt , 3\n";

                            } else if (t->isIntegerTy(32) || t->isFloatTy()) {
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << (is64 ? "": "temp") << " = ashr i64 ";
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() <<  "subt , 2\n";

                                printAsFixedOperand((*i), &myfile);
                                myfile << (is64 ? "": "temp") << " = ashr i64 ";
                                printAsFixedOperand((*i), &myfile);
                                myfile <<  "subt , 2\n";

                            } else if (t->isIntegerTy(16)) {
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << (is64 ? "": "temp") << " = ashr i64 ";
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() <<  "subt , 1\n";

                                printAsFixedOperand((*i), &myfile);
                                myfile << (is64 ? "": "temp") << " = ashr i64 ";
                                printAsFixedOperand((*i), &myfile);
                                myfile <<  "subt , 1\n";
                            } else if (t->isIntegerTy(8)) {
                                //TODO.
                                assert(0 && "not implemented yet!");
                            }

                            if(!(*i)->getType()->isIntegerTy(64)) {

                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << "= trunc i64 ";
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() <<  "temp to " << *((*i)->getType()) << "\n";

                                printAsFixedOperand((*i), &myfile);
                                myfile << "= trunc i64 ";
                                printAsFixedOperand((*i), &myfile);
                                myfile <<  "temp to " << *((*i)->getType()) << "\n";

                            }




                        } else if (StructType* st = dyn_cast<StructType>(t)) {

                            dbgs() << "%Size = getelementptr %" << (*st).getName() << ", %" << (*st).getName() << "* null, i32 1\n";
                            dbgs() << "%SizeI = ptrtoint %" << (*st).getName() << "* %Size to i64\n";


                            myfile << "%Size = getelementptr %" << (*st).getName() << ", %" << (*st).getName() << "* null, i32 1\n";
                            myfile << "%SizeI = ptrtoint %" << (*st).getName() << "* %Size to i64\n";



                            if((*i)->getType()->isIntegerTy(64)) {
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << " = udiv i64 ";
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() <<  "subt , %SizeI \n";

                                printAsFixedOperand((*i), &myfile);
                                myfile << " = udiv i64 ";
                                printAsFixedOperand((*i), &myfile);
                                myfile <<  "subt , %SizeI \n";


                            } else {
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << "temp = udiv i64 ";
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() <<  "subt , %SizeI \n";
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() << "= trunc i64 ";
                                (*i)->printAsOperand(dbgs(), false);
                                dbgs() <<  "temp to " << *((*i)->getType()) << "\n";

                                printAsFixedOperand((*i), &myfile);
                                myfile << "temp = udiv i64 ";
                                printAsFixedOperand((*i), &myfile);
                                myfile <<  "subt , %SizeI \n";
                                printAsFixedOperand((*i), &myfile);
                                myfile << "= trunc i64 ";
                                printAsFixedOperand((*i), &myfile);
                                myfile <<  "temp to " << *((*i)->getType()) << "\n";
                                //TODO: more types?
                            }
                        }
                    } else {
                        //weird Induction
                        myfile << "%addr = call i64 @get_state_data(i64 9)\n";
                        dbgs() << "%addr = call i64 @get_state_data(i64 9)\n";

                        (*i)->printAsOperand( myfile , false);
                        myfile << " = inttoptr i64 %addr to "  << *(*i)->getType() << "\n";

                    }


                } else if (p.loadRounds.count(*i) || dyn_cast<IntrinsicInst>(*i) ) {

                    assert (dyn_cast<LoadInst>(*i) || dyn_cast<IntrinsicInst>(*i));

                    User* u = dyn_cast<User>(*i);

                    u->getOperand(0)->printAsOperand(dbgs(), false);
                    dbgs() << "temp = ptrtoint ";
                    u->getOperand(0)->printAsOperand(dbgs(), true);
                    dbgs() << " to i64\n";

                    u->getOperand(0)->printAsOperand(myfile, false);
                    myfile << "temp = ptrtoint ";
                    u->getOperand(0)->printAsOperand(myfile, true);
                    myfile << " to i64\n";

                    if(p.loadRounds.count(*i)) {
                        dbgs() << "call void @set_fetching_extra(i64 ";
                        u->getOperand(0)->printAsOperand(dbgs(), false);
                        dbgs() << "temp, i64 " << std::to_string(p.loadRounds.find(*i)->second) << ")\n";

                        myfile << "call void @set_fetching_extra(i64 ";
                        u->getOperand(0)->printAsOperand(myfile, false);
                        myfile << "temp, i64 " << std::to_string(p.loadRounds.find(*i)->second) << ")\n";
                    } else {
                        dbgs() << "call void @set_fetching(i64 ";
                        u->getOperand(0)->printAsOperand(dbgs(), false);
                        dbgs() << "temp)\n";

                        myfile << "call void @set_fetching(i64 ";
                        u->getOperand(0)->printAsOperand(myfile, false);
                        myfile << "temp)\n";
                    }
                }

                else {
                    dbgs() << **i << "\n";
                    

                    //This is hacky. It replaces all of the @s, which are normally for globals but can be for functions, with %s, so that renaming works properly for the events.
                        std::string type_str;
                        llvm::raw_string_ostream s (type_str);
                        s << **i;
                        std::string st = s.str();
                        std::replace( st.begin(), st.end(), '@', '%');
              
                    myfile << st << "\n";
                }

            }


            if(p.batchMerged) {
                dbgs() << "%loopcond = icmp eq i64 %nextvar, " << p.batchSize << "\n";
                dbgs() << "br i1 %loopcond, label %afterloop, label %loop\n";

                dbgs() << "afterloop:      ; preds = %loop\n\n";

                myfile << "%loopcond = icmp eq i64 %nextvar, " << p.batchSize << "\n";
                myfile << "br i1 %loopcond, label %afterloop, label %loop\n";

                myfile << "afterloop:      ; preds = %loop\n\n";
            }


            myfile << "ret void\n}\n\n\n";


            round++;

        }

        myfile.close();


        pid_t pid = fork();

        if (pid == -1)
        {
            // error, failed to fork()
        }
        else if (pid > 0)
        {
            int status;
            waitpid(pid, &status, 0);
        }
        else
        {
            // we are the child
            execl("/bin/sed", "/bin/sed", "-r", "-i", "s/(\\%)([0-9]+)/\\1s\\2/g", "pfoutput.ll", (char *) 0);
            _exit(EXIT_FAILURE);   // exec never returns
        }



        raw_fd_ostream mefile ("pfoutput.ll", EC,sys::fs::F_Append );



        mefile << "\n\n\ndeclare i64 @get_state_data(i64) #1\ndeclare void @set_fetching(i64) #1\ndeclare void @set_fetching_extra(i64, i64) #1\n";
        mefile <<"declare void @quiesce() #1\ndeclare i64 @cont_or_susp() #1\ndeclare void @set_function_pointer(i64, i64, i64) #1\ndeclare void @add_special_target(i64, i64) #1\n\n\n";

        //mefile <<"define void @main() #0 {\n\n";

        mefile <<"define void @prefetch() #0 {\n\n";


        mefile << "tail call void @quiesce() #3\n";

        //add function pointers;

        for(int x=0; x<round; x++) {
            if(specials.count(x)) {
                mefile << "tail call void @add_special_target(i64 " << std::to_string(x) << ", i64 zext (i32 ptrtoint (void ()* @round" << std::to_string(x) << " to i32) to i64))\n";
            } else {
                mefile << "tail call void @set_function_pointer(i64 " << std::to_string(functionMap.find(x)->second) << ", i64 zext (i32 ptrtoint (void ()* @round" << std::to_string(x) << " to i32) to i64), i64 " << (rounds[x].isBatch ? "16" : "0") << ") #1\n";

            }
        }



        mefile << "br label %1\n; <label>:1                                       ; preds = %0, %5\n  %2 = tail call i64 @cont_or_susp() #3\n";
        mefile << "  %3 = icmp eq i64 %2, 0\n  br i1 %3, label %4, label %5\n\n";
        mefile << "; <label>:4                                       ; preds = %1\n  tail call void @quiesce() #3\n  br label %5\n\n";

        mefile << "; <label>:5                                       ; preds = %1, %4\n  %6 = tail call i64 @get_state_data(i64 10) #3\n";
        mefile <<"  %7 = inttoptr i64 %6 to void ()*\n  tail call void %7() #3\n  br label %1\n\n}\n";



        mefile.close();







        for (auto &p : prefetchStates) {
            p.swprefetch->dropAllReferences();
            p.swprefetch->eraseFromParent();
            modified = true;
        }


        std::set<Instruction*> instsToRemove;

        for (auto &p : prefetchStates) {
            for(int x=0; x<p.currentInstructions; x++) {
                for(auto i = p.Instructions[x].begin(); i != p.Instructions[x].end(); ++i) {
                    Value* v = *i;


                    if(v && dyn_cast<Instruction>(v)) {
                        Instruction * in = dyn_cast<Instruction>(v);
                        if(in && !instsToRemove.count(in)) {
                            if(v && v->hasNUses(0)) {
                                if(in->getParent()) {

                                    dbgs() << "removing " << (*in) << "\n";
                                    in->dropAllReferences();
                                    instsToRemove.insert(in);



                                    modified = true;
                                }
                            }
                        }
                    }
                }

            }
        }
        
        for( auto &pf : failedPFs) {
			dbgs() << "removing failed pf " << (*pf) << "\n";		
			                                    Use* u = pf->getOperandList();
									Use* end = u + pf->getNumOperands();
									

									std::vector<Instruction*> insts;
									for(;u<end; u++) {
										if(Instruction* K = dyn_cast<Instruction>(u->get()))
										  insts.push_back(K);
									}
			
			pf->dropAllReferences();
			
				
			eraseKids(insts,instsToRemove);
			pf->eraseFromParent();
			
		}

        for(auto t = instsToRemove.begin(); t != instsToRemove.end(); t++) {
            (*t)->eraseFromParent();
            
        }




        return modified;
    }
};
char ConversionPass::ID;

/// This function is called by the PassManagerBuilder to add the pass to the
/// pass manager.  You can query the builder for the optimisation level and so
/// on to decide whether to insert the pass.
void addConversionPass(const PassManagerBuilder &Builder, legacy::PassManagerBase &PM) {

	PM.add(createLoopRerollPass());
    PM.add(new ConversionPass());
    PM.add(createVerifierPass());

}

/// Register the pass with the pass manager builder.  This instructs the
/// builder to call the `addConversionPass` function at the end of adding other
/// optimisations, so that we can insert the pass.  See the
/// `PassManagerBuilder` documentation for other extension points.
RegisterStandardPasses S(PassManagerBuilder::EP_VectorizerStart ,
                         addConversionPass);
} // anonymous namespace


