#include "dct/program_generator.h"

namespace DCT {

using namespace DRAMBender;

DRAMBender::Program singleside_hammer(int bank, int aggr_id, int hammer_count, int nAggON, int nAggOFF) {
  struct Reg {
    enum : int{
      NUM_HMR = ReservedReg::Last,
      HMR_COUNTER,
      LAST
    };
  };

  Program p;
  // Load bank and row address registers
  p.add_inst(SMC_LI(bank, ReservedReg::BAR));
  p.add_inst(SMC_LI(aggr_id, ReservedReg::RAR));
  
  p.add_inst(SMC_LI(0, Reg::HMR_COUNTER));
  p.add_inst(SMC_LI(hammer_count, Reg::NUM_HMR));

  // Activate the aggressor row for hammer_count times
  p.add_label("HMR_BEGIN");

  // Additive RAS latency after 36ns standard tRAS, step size is 5 * 6 = 30ns
  if (nAggON > 1)
    p.add_inst(SMC_SLEEP(5 * (nAggON - 1)));

  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_ADDI(Reg::HMR_COUNTER, 1, Reg::HMR_COUNTER));
  // Additive RP latency after 15ns standard tRP, step size is 3 * 6 = 18ns
  if (nAggOFF > 1)
    p.add_inst(SMC_SLEEP(3 * (nAggOFF - 1)));

  p.add_inst(SMC_NOP(), SMC_NOP(), SMC_NOP(), SMC_ACT(ReservedReg::BAR, 0, ReservedReg::RAR, 0));
  p.add_branch(p.BR_TYPE::BL, Reg::HMR_COUNTER, Reg::NUM_HMR, "HMR_BEGIN");

  p.conclude();

  return p;
}
  
} // namespace DCT
