#include "dct/program_generator.h"

namespace DCT {

using namespace DRAMBender;

DRAMBender::Program act_row(int bank, int row_id, int nRAS, int nRP) {
  Program p;
  p.add_inst(SMC_LI(bank, ReservedReg::BAR));
  p.add_inst(SMC_LI(row_id, ReservedReg::RAR));
  p.add_inst(SMC_LI(8, ReservedReg::CASR));

  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_LI(0, ReservedReg::CAR));
  p.add_inst(all_nops());
  p.add_inst(all_nops());

  // Activate and close the row with nRAS and nRP
  p.add_mininst(SMC_ACT(ReservedReg::BAR, 0, ReservedReg::RAR, 0), nRAS);
  p.add_mininst(SMC_PRE(ReservedReg::BAR, 0, 0), nRP);

  // "Harden" the row with nominal tRAS (36ns) and tRP ()
  p.add_mininst(SMC_ACT(ReservedReg::BAR, 0, ReservedReg::RAR, 0), 24);
  p.add_mininst(SMC_PRE(ReservedReg::BAR, 0, 0), 0);

  p.pack_minprogram();

  p.add_inst(SMC_SLEEP(3));
  p.conclude();
  return p;
}
  
} // namespace DCT
