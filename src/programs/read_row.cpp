#include "dct/program_generator.h"

namespace DCT {

using namespace DRAMBender;

DRAMBender::Program read_row(int bank, int row_id) {
  Program p;
  // Load bank and row address registers
  p.add_inst(SMC_LI(bank, ReservedReg::BAR));
  p.add_inst(SMC_LI(row_id, ReservedReg::RAR));
  // Column address stride is 8 since we are doing BL=8
  p.add_inst(SMC_LI(8, ReservedReg::CASR));

  // Activate and read from the row
  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_LI(0, ReservedReg::CAR));
  p.add_inst(all_nops());
  p.add_inst(all_nops());

  p.add_inst(SMC_ACT(ReservedReg::BAR, 0, ReservedReg::RAR, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(all_nops()); 
  p.add_inst(all_nops());

  for(int i = 0 ; i < 128 ; i++) {
    p.add_inst(SMC_READ(ReservedReg::BAR, 0, ReservedReg::CAR, 1, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
    p.add_inst(all_nops()); 
  }
  p.add_inst(SMC_SLEEP(4));

  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_SLEEP(3));

  // p.dump_registers();

  p.conclude();
  // p.pretty_print();
  return p;
}
  
} // namespace DCT
