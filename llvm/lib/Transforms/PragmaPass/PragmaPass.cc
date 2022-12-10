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


#include <iostream>
#include <map>

#include <llvm/Support/Debug.h>

using namespace llvm;

// LLVM passes are normally defined in the anonymous namespace, as they should
// only ever be exposed via their superclass interface
namespace {


struct PragmaPass : FunctionPass, InstVisitor<PragmaPass>
{
    /// The module that we're currently working on
    Module *M = 0;
    /// The data layout of the current module.
    DataLayout *DL = 0;
    /// Unique value.  Its address is used to identify this class.
    static char ID;
    /// Call the superclass constructor with the unique identifier as the
    /// (by-reference) argument.

    PragmaPass() : FunctionPass(ID) {}


    /// Return the name of the pass, for debugging.
    const char *getPassName() const override {
        return "Xtreme Software Prefetch";
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

        return false;
    }



    virtual void getAnalysisUsage(AnalysisUsage& AU) const override {
        AU.addRequired<LoopInfoWrapperPass>();
    }
    
    
    bool makeLoopInvariantSpec(Instruction *I, bool &Changed, Loop* L) const {
   // Test if the value is already loop-invariant.
   if (L->isLoopInvariant(I))
     return true;
   // EH block instructions are immobile.
   // Determine the insertion point, unless one was given.
   if(!I) return false;
   if(!isSafeToSpeculativelyExecute(I) && !I->mayReadFromMemory()) return false; //hacky af but it works for now.
   
     BasicBlock *Preheader = L->getLoopPreheader();
     // Without a preheader, hoisting is not feasible.
     if (!Preheader)
       return false;
     Instruction* InsertPt = Preheader->getTerminator();
   
   // Don't hoist instructions with loop-variant operands.
   for (Value *Operand : I->operands())
     if(Instruction* i = dyn_cast<Instruction>(Operand)) if (!makeLoopInvariantSpec(i, Changed, L)) {
	   Changed = false;
       return false;
   }
 
   // Hoist.
   I->moveBefore(InsertPt);
 
   // There is possibility of hoisting this instruction above some arbitrary
   // condition. Any metadata defined on it can be control dependent on this
   // condition. Conservatively strip it here so that we don't give any wrong
   // information to the optimizer.
 
   Changed = true;
   return true;
 }
    
    Value *getCanonicalishSizeVariable(Loop* L) const {

        // Loop over all of the PHI nodes, looking for a canonical indvar.

        //Value* indVar = getCanonicalishInductionVariable(L);

        auto B = L->getExitingBlock();
        
        if(!B) return nullptr;




        for(Instruction &J : *B) {

            Instruction* I = &J;
            CmpInst *CI = dyn_cast<CmpInst>(I);

            //dbgs() << *I;
            //if (ConstantInt *CI =
            //  dyn_cast<ConstantInt>(PN->getIncomingValueForBlock(Incoming)))
            //if (CI->isNullValue())

            if(CI) {
                if(L->isLoopInvariant(CI->getOperand(1))) return CI->getOperand(1);

                if(L->isLoopInvariant(CI->getOperand(0))) return CI->getOperand(0);

                dbgs() << "Size not loop invariant!\n";
            }



        }



        return nullptr;
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
                        //if (CI->equalsInt(1))
                            return PN;
        }
        return nullptr;
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
    
        Value *getOddPhiFirst(Loop* L, PHINode* PN) const {
        BasicBlock *H = L->getHeader();

        BasicBlock *Incoming = nullptr, *Backedge = nullptr;
        pred_iterator PI = pred_begin(H);
        assert(PI != pred_end(H) &&
               "Loop must have at least one backedge!");
        Backedge = *PI++;
        if (PI == pred_end(H)) return nullptr;  // dead loop
        Incoming = *PI++;
        if (PI != pred_end(H)) return nullptr;  // multiple backedges?
        
        if(H != PN->getParent()) return nullptr;

        if (L->contains(Incoming)) {
            if (L->contains(Backedge))
                return nullptr;
            std::swap(Incoming, Backedge);
        } else if (!L->contains(Backedge))
            return nullptr;

        // Loop over all of the PHI nodes, looking for a canonical indvar.
            //if (ConstantInt *CI =
            //  dyn_cast<ConstantInt>(PN->getIncomingValueForBlock(Incoming)))
            //if (CI->isNullValue())
            return PN->getIncomingValueForBlock(Incoming);

    }
    
    
    
    
    bool depthFirstSearch (Instruction* I, LoopInfo &LI, Instruction* &Phi, SmallVector<Instruction*,8> &Instrs) {
		    Use* u = I->getOperandList();
            Use* end = u + I->getNumOperands();
            
            SmallVector<Instruction*,8> roundInsts;
            SmallSet<Instruction*,8> gotPhi;
            
            bool found = false;
            
            for(Use* v = u; v<end; v++) {
				
				PHINode* p = dyn_cast<PHINode>(v->get());
				Loop* L = nullptr;
				if(p) L = LI.getLoopFor(p->getParent());
				
				if(p && L && (p == getCanonicalishInductionVariable (L) || p == getWeirdCanonicalishInductionVariable(L))) {
				
					
						//dbgs() << *p << "\n";
						
						dbgs() << "Loop induction phi node! " << *p << "\n";
						
						if(Phi) {
								if(Phi == p) {
									//add this
									roundInsts.push_back(p);
									gotPhi.insert(p);
									found = true; //should have been before anyway
								} else {
									//check which is older.
									if(LI.getLoopFor(Phi->getParent())->isLoopInvariant(p)) {
										//do nothing
										dbgs() << "not switching phis\n";
									} else if (LI.getLoopFor(p->getParent())->isLoopInvariant(Phi)) {
										dbgs() << "switching phis\n";
										roundInsts.clear();
										roundInsts.push_back(p);
										Phi = p;
										gotPhi.clear();
										gotPhi.insert(p);
										found = true;
									} else {
										//OMG HALP IS DISS EVEN POSSIBL!???
										dbgs() << "OMG HALP\n";
									}
								}
							} else {
								
								Phi = p;
								roundInsts.push_back(p);
								gotPhi.insert(p);
								found = true;
							}
						
						
					
				}
				else if(dyn_cast<StoreInst>(v->get())) {}
				else if(dyn_cast<CallInst>(v->get())) {}
				else if(dyn_cast<TerminatorInst>(v->get())) {}
				else if(Instruction* k = dyn_cast<Instruction>(v->get())) {
					
					if(!((!p) || L != nullptr)) continue;
				
					Instruction* j = k;
				
					Loop* L = LI.getLoopFor(j->getParent());
					if(L) {
						
						SmallVector<Instruction*,8> Instrz;
						
						
						
						if(p) {
							dbgs() << "Non-loop-induction phi node! " << *p << "\n";
							j = nullptr;
							//continue; //TODO: remove for advanced mode
							if(!getOddPhiFirst(L,p)) {return false;}
							j = dyn_cast<Instruction> (getOddPhiFirst(L,p));
							if(!j) {return false;}
							Instrz.push_back(k);
							Instrz.push_back(j);
							L = LI.getLoopFor(j->getParent());
							
						} else Instrz.push_back(k);
						
						
						
						
						//dbgs() << *j << "\n";
						Instruction * phi = nullptr;
						if(j && depthFirstSearch(j,LI,phi, Instrz)) {
							if(Phi) {
								if(Phi == phi) {
									//add this
									for(auto q : Instrz) roundInsts.push_back(q);
									gotPhi.insert(j);
									found = true; //should have been before anyway
								} else {
									//check which is older.
									if(LI.getLoopFor(Phi->getParent())->isLoopInvariant(phi)) {
										//do nothing
										dbgs() << "not switching phis\n";
									} else if (LI.getLoopFor(phi->getParent())->isLoopInvariant(Phi)) {
										dbgs() << "switching phis\n";
										roundInsts.clear();
										for(auto q : Instrz) roundInsts.push_back(q);
										Phi = phi;
										gotPhi.clear();
										gotPhi.insert(j);
										found = true;
									} else {
										//OMG HALP IS DISS EVEN POSSIBL!???
										dbgs() << "OMG HALP\n";
									}
								}
							} else {
								for(auto q : Instrz) roundInsts.push_back(q);
								Phi = phi;
								gotPhi.insert(j);
								found = true;
							}
						}
					}
				}
			}
			
			if(false && Phi && found) { //TODO: doesn't work properly...
			for(Use* v = u; v<end; v++) {
				Value* val = v->get();
				
				Instruction* inst = dyn_cast<Instruction>(val);
				
				if(inst && gotPhi.count(inst)) {
					
				} else if(LI.getLoopFor(Phi->getParent())->isLoopInvariant(val))
				{
				} else {
					found = false;
					dbgs() << "non-loop invariant operand: " << *val << "\n";
				}
			}
		}
			
			if(found) for(auto q : roundInsts) Instrs.push_back(q);
			
			return found;
	}



    bool runOnFunction(Function &F) override {
		
		
		auto global_annos = M->getNamedGlobal("llvm.global.annotations");
if (global_annos) {
  auto a = cast<ConstantArray>(global_annos->getOperand(0));
  for (uint64_t i=0; i<a->getNumOperands(); i++) {
    auto e = cast<ConstantStruct>(a->getOperand(i));

    if ((&F) == dyn_cast<Function>(e->getOperand(0)->getOperand(0))) {
      auto anno = cast<ConstantDataArray>(cast<GlobalVariable>(e->getOperand(1)->getOperand(0))->getOperand(0))->getAsCString();
      F.addFnAttr(anno); // <-- add function annotation here
    }
  }
}

  if (F.hasFnAttribute("prefetch")) {
    dbgs() << F.getName() << " has prefetch attribute!\n";
  } else return false;

			
		LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

        bool modified = false;
        
        	SmallVector<Instruction*,4> Loads;
			SmallVector<Instruction*,4> Phis;
			std::vector<SmallVector<Instruction*,8>> Insts;
        
        for(auto &BB : F) {
			
		
			
			for (auto &I : BB) {
				if (LoadInst *i = dyn_cast<LoadInst>(&I)) {
					if(LI.getLoopFor(&BB)) {
						SmallVector<Instruction*,8> Instrz;
						Instrz.push_back(i);
						Instruction * phi = nullptr;
						if(depthFirstSearch(i,LI,phi,Instrz)) {
							
							int loads = 0;
							for(auto &z : Instrz) {
								if(dyn_cast<LoadInst>(z)) {
									loads++;
								}
							}
							
							//if(loads < 2) { dbgs()<<"stride\n";continue; }//remove the stride cases.
							
							dbgs() << "Can prefetch " << *i << " from PhiNode " << *phi << "\n";
							dbgs() << "need to change \n";
							for (auto &z : Instrz) {
								dbgs() << *z << "\n";
							}
							
							Loads.push_back(i);
							Insts.push_back(Instrz);
							Phis.push_back(phi);
		
							} 
						 else {
							dbgs() << "Can't prefetch load" << *i << "\n";
						}
						
					}
					}
				}
				
			
		}
		
		
		
						for(uint64_t x=0; x<Loads.size(); x++) {
						ValueMap<Instruction*, Value*> Transforms; //TODO: this won't work properly for multilevel prefetches. Need to add in a parameter for fetch distance here too, for indexing.

						bool ignore = false;
						
						for(uint64_t y=x+1; y< Loads.size(); y++) {
							bool subset = true;
							for(auto &in : Insts[x]) {
								if(std::find(Insts[y].begin(),Insts[y].end(),in) == Insts[y].end()) subset = false;
							}
							if(subset) {ignore = true; break;}
						}
						
						if(ignore) continue;
							
							IRBuilder<> Builder(Loads[x]);
							
							bool tryToPushDown = (LI.getLoopFor(Loads[x]->getParent()) != LI.getLoopFor(Phis[x]->getParent()));
							
							if(tryToPushDown) dbgs() << "Trying to push down!\n";
							
							
							
										
							
	
								//reverse list.
								SmallVector<Instruction*,8> newInsts;
								for(auto q = Insts[x].end()-1; q > Insts[x].begin()-1; q--) newInsts.push_back(*q);
								for(auto &z : newInsts) {
									if(Transforms.count(z)) continue;
									
								
									
									if(z == Phis[x]) { 
										
										Instruction* n;
										
										bool weird = false;
									    
									     Loop* L = LI.getLoopFor(Phis[x]->getParent());
									     
									     int offset = 64;
									    
									    if(z == getCanonicalishInductionVariable(L)) n = dyn_cast<Instruction>(Builder.CreateAdd(Phis[x],Phis[x]->getType()->isIntegerTy(64) ? ConstantInt::get(Type::getInt64Ty(M->getContext()),offset) : ConstantInt::get(Type::getInt32Ty(M->getContext()),offset)));
									    else if (z == getWeirdCanonicalishInductionVariable(L)) {
											n = getWeirdCanonicalishInductionVariableGep(L)->clone();
											Builder.Insert(n);
											n->setOperand (n->getNumOperands ()-1, ConstantInt::get(Type::getInt64Ty(M->getContext()),offset));
											weird = true;
											
											bool changed = true; 
											while(LI.getLoopFor(Phis[x]->getParent()) != LI.getLoopFor(n->getParent()) && changed) {
												 
												 Loop* ol = LI.getLoopFor(n->getParent());
												 
												 makeLoopInvariantSpec(n,changed,LI.getLoopFor(n->getParent()));
												 
												 if(ol && ol == LI.getLoopFor(n->getParent())) break;
												 
												 
											} 
											
										}
										
			    
									    
									   
									    assert(L);
									    assert(n);
									    
									    Value* size = getCanonicalishSizeVariable(L); 
									    if(true || /*weird || */!size || !size->getType()->isIntegerTy()) {
											Transforms.insert(std::pair<Instruction*,Instruction*>(z,n));
											continue; 
										}
											 
										
										Instruction* mod;	 
										
										if(weird) {
											Instruction* endcast = dyn_cast<Instruction>(Builder.CreatePtrToInt(size,Type::getInt64Ty(M->getContext())));
									
											
											Instruction* startcast =  dyn_cast<Instruction>(Builder.CreatePtrToInt(getWeirdCanonicalishInductionVariableFirst(L),Type::getInt64Ty(M->getContext())));
										
											
											Instruction* valcast =  dyn_cast<Instruction>(Builder.CreatePtrToInt(n,Type::getInt64Ty(M->getContext())));
								
											
											Instruction* sub1 = dyn_cast<Instruction>(Builder.CreateSub(valcast,startcast));
											Instruction* sub2 = dyn_cast<Instruction>(Builder.CreateSub(endcast,startcast));
											
											//Instruction* rem = dyn_cast<Instruction>(Builder.CreateURem(sub1,sub2));
											
											Value* cmp = Builder.CreateICmp(CmpInst::ICMP_SLT,sub1,sub2);
											Instruction* rem = dyn_cast<Instruction>(Builder.CreateSelect(cmp,sub1,sub2));
											
											Instruction* add = dyn_cast<Instruction>(Builder.CreateAdd(rem,startcast));
											
											mod = dyn_cast<Instruction>(Builder.CreateIntToPtr(add,n->getType()));
											
											
											
											
											
											
										}											 
										else if(size->getType() != n->getType()) {
											Instruction* cast = CastInst::CreateIntegerCast(size,Type::getInt64Ty(M->getContext()),true);
											assert(cast);
											Builder.Insert(cast);
											Value* cmp = Builder.CreateICmp(CmpInst::ICMP_SLT,cast,n);
											mod = dyn_cast<Instruction>(Builder.CreateSelect(cmp,cast,n));
											//mod =  dyn_cast<Instruction>(Builder.CreateURem(n,cast));
										} else {
											//mod = dyn_cast<Instruction>(Builder.CreateURem(n,size));
											Value* cmp = Builder.CreateICmp(CmpInst::ICMP_SLT,size,n);
											mod = dyn_cast<Instruction>(Builder.CreateSelect(cmp,size,n));
										}
									    
									    
									    											bool changed = true; 
											while(LI.getLoopFor(Phis[x]->getParent()) != LI.getLoopFor(mod->getParent()) && changed) {
												Loop* ol = LI.getLoopFor(mod->getParent());
												  makeLoopInvariantSpec(mod,changed,LI.getLoopFor(mod->getParent()));
												 if(ol && ol == LI.getLoopFor(mod->getParent())) break;
											} 
									    
									    Transforms.insert(std::pair<Instruction*,Instruction*>(z,mod));	
									    modified = true;
								    } else if (z == Loads[x]) {
																				
										Function *fun = Intrinsic::getDeclaration(F.getParent(), Intrinsic::prefetch);
										
										assert(fun);
										
										Instruction* oldGep = dyn_cast<Instruction>(Loads[x]->getOperand(0));
										assert(oldGep);
										
										Instruction* gep = dyn_cast<Instruction>(Transforms.lookup(oldGep));
										assert(gep);
										modified = true;
										
										
										
										Instruction* cast = dyn_cast<Instruction>(Builder.CreateBitCast (gep, Type::getInt8PtrTy(M->getContext())));
										
																					bool changed = true; 
											while(LI.getLoopFor(Phis[x]->getParent()) != LI.getLoopFor(cast->getParent()) && changed) {
												Loop* ol = LI.getLoopFor(cast->getParent());
												  makeLoopInvariantSpec(cast,changed,ol);
												 if(ol && ol == LI.getLoopFor(cast->getParent())) break;
											} 
										
										
										Value* ar[] = {
										cast,
										ConstantInt::get(Type::getInt32Ty(M->getContext()),0),
										ConstantInt::get(Type::getInt32Ty(M->getContext()),3),
										ConstantInt::get(Type::getInt32Ty(M->getContext()),1)
										};
										
										
										//Instruction* call = Builder.CreateCall(fun, ar); //TODO
										
										CallInst* call = CallInst::Create(fun,ar);
                            //cast->getParent()->getInstList().insert(cast->getParent()->getTerminator(),call);
                           call->insertBefore(cast->getParent()->getTerminator());
									
										
										
										
										
									} else if(PHINode * pn = dyn_cast<PHINode>(z)) {
										
										Value* v = getOddPhiFirst(LI.getLoopFor(pn->getParent()),pn);
										if(Instruction* ins = dyn_cast<Instruction>(v)) v = Transforms.lookup(ins);
										Transforms.insert(std::pair<Instruction*,Value*>(z,v));	
									}									
									else {
										
										Instruction* n = z->clone();
										
										Use* u = n->getOperandList();
										int64_t size = n->getNumOperands();
										for(int64_t x = 0; x<size; x++) {
											Value* v = u[x].get();
											if(Instruction* t = dyn_cast<Instruction>(v)) {
												if(Transforms.count(t)) {
													n->setOperand(x,Transforms.lookup(t));
												}
											}
										}
										
										//Loads[x]->getParent()->getInstList().insert(Loads[x],n);
										n->insertBefore(Loads[x]);
										
											bool changed = true; 
											while(changed && LI.getLoopFor(Phis[x]->getParent()) != LI.getLoopFor(n->getParent())) {
												changed = false;
												
												 makeLoopInvariantSpec(n,changed,LI.getLoopFor(n->getParent()));
												 if(changed) dbgs()<< "moved loop" << *n << "\n";
												 else dbgs()<< "not moved loop" << *n << "\n";
												 //if(ol && ol == LI.getLoopFor(n->getParent())) break;
											} 
										
										Transforms.insert(std::pair<Instruction*,Instruction*>(z,n));	
										modified = true;
									}
									
								}
				
	
		}
        
        return modified;
    }

};


char PragmaPass::ID;



/// This function is called by the PassManagerBuilder to add the pass to the
/// pass manager.  You can query the builder for the optimisation level and so
/// on to decide whether to insert the pass.
void addPragmaPass(const PassManagerBuilder &Builder, legacy::PassManagerBase &PM) {
	PM.add(createLoopRerollPass());
    PM.add(new PragmaPass());
    PM.add(createVerifierPass());

}

/// Register the pass with the pass manager builder.  This instructs the
/// builder to call the `addSimplePass` function at the end of adding other
/// optimisations, so that we can insert the pass.  See the
/// `PassManagerBuilder` documentation for other extension points.
RegisterStandardPasses S(PassManagerBuilder::EP_VectorizerStart    ,
                         addPragmaPass);


}
