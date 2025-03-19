#include "dct/program_generator.h"

namespace DCT {

using namespace DRAMBender;

DRAMBender::Program refresh_all_rows(int bank, int num_rows) {
  struct Reg {
    enum : int{
      NUM_ROWS = ReservedReg::Last,
      Last,
    };
  };
  Program p;
  p.add_inst(SMC_LI(bank, ReservedReg::BAR));
  p.add_inst(SMC_LI(0, ReservedReg::RAR));
  p.add_inst(SMC_LI(num_rows, Reg::NUM_ROWS));
  p.add_inst(SMC_LI(8, ReservedReg::CASR));

  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_LI(0, ReservedReg::CAR));
  p.add_inst(all_nops());
  p.add_inst(SMC_NOP(), SMC_NOP(), SMC_NOP(), SMC_ACT(ReservedReg::BAR, 0, ReservedReg::RAR, 0));
  p.add_inst(SMC_SLEEP(6));

  p.add_label("ACT_ROW_BEGIN");
  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(all_nops());
  p.add_inst(SMC_ADDI(ReservedReg::RAR, 1, ReservedReg::RAR));
  p.add_inst(SMC_NOP(), SMC_NOP(), SMC_NOP(), SMC_ACT(ReservedReg::BAR, 0, ReservedReg::RAR, 0));
  p.add_branch(p.BR_TYPE::BL, ReservedReg::RAR, Reg::NUM_ROWS, "ACT_ROW_BEGIN");
  
  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_SLEEP(3));

  p.conclude();
  return p;
}
  
} // namespace DCT
