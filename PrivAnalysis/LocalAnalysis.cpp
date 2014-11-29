// ====--------------  LocalAnalysis.cpp ---------*- C++ -*---====
//
// Local analysis of priv_lower calls in Function blocks.
// Find all the priv_lower calls inside each of the functions.
//
// ====-------------------------------------------------------====

#include "LocalAnalysis.h"
#include "SplitBB.h"

#include <linux/capability.h>
#include <map>
#include <array>

using namespace llvm;
using namespace llvm::privAnalysis;
using namespace llvm::splitBB;
using namespace llvm::localAnalysis;


// Constructor
LocalAnalysis::LocalAnalysis() : ModulePass(ID) {}


// Do initialization
// param: Module
bool LocalAnalysis::doInitialization(Module &M){
  return false;
}


// RetrieveAllCAP
// Retrieve all capabilities from params of function call
// param: CI - call instruction to retrieve from
//        CAParray - the array of capability to save to
void LocalAnalysis::RetrieveAllCAP(CallInst *CI, CAPArray_t &CAParray){
  int numArgs = (int) CI->getNumArgOperands();

  // Note: Skip the first param of priv_lower for it's num of args
  for (int i = 1; i < numArgs; i ++){
    // retrieve integer value
    Value *v = CI->getArgOperand(i);
    ConstantInt *I = dyn_cast<ConstantInt>(v);
    unsigned int iarg = I->getZExtValue();

    // Add it to the array
    CAParray[iarg] = 1;
  }
}


// Run on Module start
// param: Module
bool LocalAnalysis::runOnModule(Module &M){

  errs() << "\nRunning Local Analysis Pass\n\n";

  // retrieve all data for later use
  SplitBB &SB = getAnalysis<SplitBB>();
  BBFuncTable = SB.BBFuncTable;
  
  // find all users of Targeted function
  Function *F = M.getFunction(PRIVRAISE);

  // Protector: didn't find any function TARGET_FUNC
  if (F == NULL){
    errs() << "Didn't find function " << PRIVRAISE << "\n";
    return false;
  }

  // Find all user instructions of function in the module
  for (Value::user_iterator UI = F->user_begin(), UE = F->user_end();
       UI != UE;
       ++UI){
    // If it's a call Inst calling the targeted function
    CallInst *CI = dyn_cast<CallInst>(*UI);
    if (CI == NULL || CI->getCalledFunction() != F){
      continue;
    }

    // Retrieve all capabilities from params of function call
    CAPArray_t CAParray = {0};
    RetrieveAllCAP(CI, CAParray);

    // Get the function where the Instr is in
    // Add CAP to Map (Function* => array of CAPs)
    // and Map (BB * => array of CAPs)
    AddToBBCAPTable(BBCAPTable, CI->getParent(), CAParray);
    AddToFuncCAPTable(FuncCAPTable, CI->getParent()->getParent(), CAParray);

  }

  // DEBUG purpose: dump map table
  // ---------------------//
  dumpCAPTable (FuncCAPTable);
  // ----------------------//

  return false;
}


// getAnalysisUsage function
// preserve all analyses
// param: AU
void LocalAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {

  AU.addRequired<SplitBB>();

  AU.setPreservesAll();
}


// Pass registry
char LocalAnalysis::ID = 0;
static RegisterPass<LocalAnalysis> A("LocalAnalysis", "Local Privilege Analysis", true, true);

