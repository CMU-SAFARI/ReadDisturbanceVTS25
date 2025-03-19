#include "dct/program_generator.h"

namespace DCT {

using namespace DRAMBender;

DRAMBender::Program read_rows(int bank, const std::vector<int>& row_ids) {
  struct Reg {
    enum : int{
      NUM_ROWS = ReservedReg::Last,
      ROW_PTR,
      ROW_ADDR,
      Last,
    };
  };

  Program p;
  // Store the row addresses into SoftMC memory
  p.add_inst(SMC_LI(row_ids.size(), Reg::NUM_ROWS));
  p.add_inst(SMC_LI(0, Reg::ROW_PTR));

  // Store the row ID and cacheline into DRAM Bender memory
  for (size_t i = 0; i < row_ids.size(); i++) {
    p.add_inst(SMC_LI(row_ids[i], Reg::ROW_ADDR));
    p.add_inst(SMC_ST(Reg::ROW_PTR, i, Reg::ROW_ADDR));
  }

  p.add_inst(SMC_LI(bank, ReservedReg::BAR));
  p.add_inst(SMC_LI(8, ReservedReg::CASR));

  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(all_nops());
  p.add_inst(all_nops());

  p.add_label("RD_ROW_BEGIN");
  p.add_inst(SMC_LD(Reg::ROW_PTR, 0, ReservedReg::RAR));

  p.add_inst(SMC_ACT(ReservedReg::BAR, 0, ReservedReg::RAR, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_LI(0, ReservedReg::CAR));
  p.add_inst(all_nops());

  for(int i = 0 ; i < 128 ; i++) {
    p.add_inst(SMC_READ(ReservedReg::BAR, 0, ReservedReg::CAR, 1, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
    p.add_inst(all_nops()); 
  }
  p.add_inst(SMC_SLEEP(3));
  p.add_inst(SMC_ADDI(Reg::ROW_PTR, 1, Reg::ROW_PTR));

  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_branch(p.BR_TYPE::BL, Reg::ROW_PTR, Reg::NUM_ROWS, "RD_ROW_BEGIN");

  p.conclude();
  return p;
}
  
} // namespace DCT
