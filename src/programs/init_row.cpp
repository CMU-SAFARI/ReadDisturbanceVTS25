#include "dct/program_generator.h"

namespace DCT {

using namespace DRAMBender;

DRAMBender::Program init_row(int bank, int row_id, const WritePattern_t& wr_pattern) {
  Program p;
  // Load bank and row address registers
  p.add_inst(SMC_LI(bank, ReservedReg::BAR));
  p.add_inst(SMC_LI(row_id, ReservedReg::RAR));
  // Column address stride is 8 since we are doing BL=8
  p.add_inst(SMC_LI(8, ReservedReg::CASR));

  // Load the cache line into the wide data register
  for(uint i = 0 ; i < 16 ; i++) {
    p.add_inst(SMC_LI(wr_pattern[i], ReservedReg::PATTERN_REG));
    p.add_inst(SMC_LDWD(ReservedReg::PATTERN_REG, i));
  }

  // Activate and write to the row
  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_LI(0, ReservedReg::CAR));
  p.add_inst(all_nops()); 
  p.add_inst(all_nops()); 

  p.add_inst(SMC_ACT(ReservedReg::BAR, 0, ReservedReg::RAR, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(all_nops()); 
  p.add_inst(all_nops());

  // Write to 128 cachelines in a 8KB row
  for(int i = 0 ; i < 128 ; i++) {
    p.add_inst(SMC_WRITE(ReservedReg::BAR, 0, ReservedReg::CAR, 1, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
    p.add_inst(all_nops()); // Account for tCCDL
  }
  p.add_inst(SMC_SLEEP(4));
  
  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(all_nops()); 
  p.add_inst(all_nops());

  // p.dump_registers();

  p.conclude();
  return p;
}
  
} // namespace DCT
