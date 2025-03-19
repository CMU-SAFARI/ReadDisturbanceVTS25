#include "dct/program_generator.h"

namespace DCT {

using namespace DRAMBender;

DRAMBender::Program rowclone(int bank, int src_id, int dst_id, int nT1) {
  struct Reg {
    enum : int{
      SRC_ROW = ReservedReg::Last,
      DST_ROW,
    };
  };

  Program p;
  p.add_inst(SMC_LI(bank, ReservedReg::BAR));
  p.add_inst(SMC_LI(src_id, Reg::SRC_ROW));
  p.add_inst(SMC_LI(dst_id, Reg::DST_ROW));
  p.add_inst(SMC_LI(8, ReservedReg::CASR));

  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_LI(0, ReservedReg::CAR));
  p.add_inst(all_nops());
  p.add_inst(all_nops());

  // Activate the 
  p.add_mininst(SMC_ACT(ReservedReg::BAR, 0, Reg::SRC_ROW, 0), 24);
  p.add_mininst(SMC_PRE(ReservedReg::BAR, 0, 0), nT1);

  // "Harden" the row with nominal tRAS (36ns) and tRP ()
  p.add_mininst(SMC_ACT(ReservedReg::BAR, 0, Reg::DST_ROW, 0), 24);
  p.add_mininst(SMC_PRE(ReservedReg::BAR, 0, 0), 0);

  p.pack_minprogram();

  p.add_inst(SMC_SLEEP(3));
  p.conclude();
  return p;
}
  
} // namespace DCT
