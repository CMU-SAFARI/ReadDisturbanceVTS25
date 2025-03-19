#include "dct/program_generator.h"

namespace DCT {

using namespace DRAMBender;

DRAMBender::Program multirow_act(int bank, const Row& rowA, const Row& rowB, int nT1, int nT2) {
  struct Reg {
    enum : int{
      ROW_A = ReservedReg::Last,
      ROW_B,
    };
  };

  Program p;
  p.add_inst(SMC_LI(bank, ReservedReg::BAR));
  p.add_inst(SMC_LI(rowA.logical_id, Reg::ROW_A));
  p.add_inst(SMC_LI(rowB.logical_id, Reg::ROW_B));
  p.add_inst(SMC_LI(8, ReservedReg::CASR));

  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_LI(0, ReservedReg::CAR));
  p.add_inst(all_nops());
  p.add_inst(all_nops());

  // Activate the 
  p.add_mininst(SMC_ACT(ReservedReg::BAR, 0, Reg::ROW_A, 0), nT1);
  p.add_mininst(SMC_PRE(ReservedReg::BAR, 0, 0), nT2);

  // "Harden" the row with nominal tRAS (36ns) and tRP ()
  p.add_mininst(SMC_ACT(ReservedReg::BAR, 0, Reg::ROW_B, 0), 24);
  p.add_mininst(SMC_PRE(ReservedReg::BAR, 0, 0), 0);

  p.pack_minprogram();

  p.add_inst(SMC_SLEEP(3));
  p.conclude();
  return p;
}
  
} // namespace DCT
