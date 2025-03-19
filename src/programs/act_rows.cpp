#include "dct/program_generator.h"

namespace DCT {

using namespace DRAMBender;

DRAMBender::Program act_rows(int bank, const std::vector<int>& row_ids, int nRAS, int nRP) {
  struct Reg {
    enum : int{
      NUM_ROWS = ReservedReg::Last,
      ROW_PTR,
      ROW_ADDR,
      Last,
    };
  };

  Program p;
  p.add_inst(SMC_LI(bank, ReservedReg::BAR));
  p.add_inst(SMC_LI(8, ReservedReg::CASR));

  // Store the row addresses into SoftMC memory
  p.add_inst(SMC_LI(row_ids.size(), Reg::NUM_ROWS));
  p.add_inst(SMC_LI(0, Reg::ROW_PTR));

  // Store the row ID and cacheline into DRAM Bender memory
  for (size_t i = 0; i < row_ids.size(); i++) {
    p.add_inst(SMC_LI(row_ids[i], Reg::ROW_ADDR));
    p.add_inst(SMC_ST(Reg::ROW_PTR, i, Reg::ROW_ADDR));
  }

  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_LI(0, ReservedReg::CAR));
  p.add_inst(all_nops());
  p.add_inst(all_nops());

  p.add_label("ACT_ROW_BEGIN");
  p.add_inst(SMC_LD(Reg::ROW_PTR, 0, ReservedReg::RAR));

  // Activate and close the row with nRAS and nRP
  p.add_mininst(SMC_ACT(ReservedReg::BAR, 0, ReservedReg::RAR, 0), nRAS);
  p.add_mininst(SMC_PRE(ReservedReg::BAR, 0, 0), nRP);

  // "Harden" the row with nominal tRAS (36ns) and tRP ()
  p.add_mininst(SMC_ACT(ReservedReg::BAR, 0, ReservedReg::RAR, 0), 24);
  p.add_mininst(SMC_PRE(ReservedReg::BAR, 0, 0), 0);

  p.pack_minprogram();
  p.add_inst(SMC_ADDI(Reg::ROW_PTR, 1, Reg::ROW_PTR));

  p.add_branch(p.BR_TYPE::BL, Reg::ROW_PTR, Reg::NUM_ROWS, "ACT_ROW_BEGIN");

  p.conclude();
  return p;
}
  
} // namespace DCT
