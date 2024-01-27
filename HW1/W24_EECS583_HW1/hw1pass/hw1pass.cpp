#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instruction.h"
#include <string>
#include <vector>
#include  <iostream>
#include "llvm/Support/Format.h"

using namespace llvm;

namespace {

struct HW1Pass : public PassInfoMixin<HW1Pass> {

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    // These variables contain the results of some analysis passes.
    // Go through the documentation to see how they can be used.
    llvm::BlockFrequencyAnalysis::Result &bfi = FAM.getResult<BlockFrequencyAnalysis>(F);
    llvm::BranchProbabilityAnalysis::Result &bpi = FAM.getResult<BranchProbabilityAnalysis>(F);
    double DynOpCount = 0, Biased_Br = 0, UnBiased_Br = 0, I_ALU = 0, FP_ALU = 0, Mem = 0, Others = 0;
    for(BasicBlock &BB : F){
      std::optional<uint64_t> BB_Freq = bfi.getBlockProfileCount(&BB, false);
      // errs() << BB_Freq << "\n";
      int DynOpCount_BB = 0, Biased_Br_BB = 0, UnBiased_Br_BB = 0, I_ALU_BB = 0, FP_ALU_BB = 0, Mem_BB = 0, Others_BB = 0;
      for(Instruction &I : BB){
        std::string opcodeName = I.getOpcodeName();
        // errs() << opcodeName << "\n";
        if (opcodeName == "br" || opcodeName == "switch" || opcodeName == "indirectbr") {
            std::vector<double> succ_freq;
            for (auto Succ = llvm::succ_begin(&BB), End = llvm::succ_end(&BB); Succ != End; ++Succ) {
              BranchProbability BranchProb = bpi.getEdgeProbability(&BB, *Succ);
              double Num = BranchProb.getNumerator();
              double Den = BranchProb.getDenominator();
              double BranchFreq = Num/Den;
              succ_freq.push_back(BranchFreq);
            }
            
            double maxElement = succ_freq[0];  // Assume the first element is the maximum
            if (!succ_freq.empty()) {
              for (size_t i = 1; i < succ_freq.size(); ++i) {
                  if (succ_freq[i] > maxElement) {
                      maxElement = succ_freq[i];
                  }
              }
            }
            // errs() << maxElement<< "\n";
            if(maxElement < 0.8){
              UnBiased_Br_BB = UnBiased_Br_BB + 1;
            }
            else{
              Biased_Br_BB = Biased_Br_BB + 1;
            }
        } 
        else if (opcodeName == "add" || opcodeName == "sub" || opcodeName == "mul" ||
                  opcodeName == "udiv" || opcodeName == "sdiv" || opcodeName == "urem" ||
                  opcodeName == "shl" || opcodeName == "lshr" || opcodeName == "ashr" ||
                  opcodeName == "and" || opcodeName == "or" || opcodeName == "xor" ||
                  opcodeName == "icmp" || opcodeName == "srem") {
            I_ALU_BB = I_ALU_BB + 1;
        } 
        else if (opcodeName == "fadd" || opcodeName == "fsub" || opcodeName == "fmul" ||
                  opcodeName == "fdiv" || opcodeName == "frem" || opcodeName == "fcmp") {
            FP_ALU_BB = FP_ALU_BB + 1;
        } 
        else if (opcodeName == "alloca" || opcodeName == "load" || opcodeName == "store" ||
                  opcodeName == "getelementptr" || opcodeName == "fence" ||
                  opcodeName == "atomiccmpxchg" || opcodeName == "atomicrmw") {
            Mem_BB = Mem_BB + 1;
        } 
        else {
            Others_BB = Others_BB + 1;
        }
        DynOpCount_BB = DynOpCount_BB + 1;

      }
      // errs() << "DynOPCount_BB: " << DynOpCount_BB<< "\n";
      if(BB_Freq.has_value()) {
        DynOpCount = DynOpCount + BB_Freq.value()*DynOpCount_BB;
        I_ALU = I_ALU + BB_Freq.value()*I_ALU_BB;
        FP_ALU = FP_ALU + BB_Freq.value()*FP_ALU_BB;
        Mem = Mem + BB_Freq.value()*Mem_BB;
        UnBiased_Br = UnBiased_Br + UnBiased_Br_BB*(BB_Freq.value());
        Biased_Br = Biased_Br + BB_Freq.value()*Biased_Br_BB;
        Others = Others + BB_Freq.value()*Others_BB;
      }
        
    }
    // errs() << (double)I_ALU/(double)DynOpCount <<" "<< Others<<" "<<I_ALU<<" "<<FP_ALU<<" "<<Mem<<" "<<UnBiased_Br<<" "<<Biased_Br<< "\n";
    if(DynOpCount==0){
      errs() << F.getName().str()<<", " << format("0, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000") << "\n";
    }
    else{
      errs() << F.getName().str() << ", " << format("%.0f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f", DynOpCount, I_ALU/DynOpCount, 
                      FP_ALU/DynOpCount, Mem/DynOpCount, Biased_Br/DynOpCount, UnBiased_Br/DynOpCount, Others/DynOpCount) << "\n";
    }
  
    return PreservedAnalyses::all();
  }
    
};
}


extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "HW1Pass", "v0.1",
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM,
        ArrayRef<PassBuilder::PipelineElement>) {
          if(Name == "hw1"){
            FPM.addPass(HW1Pass());
            return true;
          }
          return false;
        }
      );
    }
  };
}