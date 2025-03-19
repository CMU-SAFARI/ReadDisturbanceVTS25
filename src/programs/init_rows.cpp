#include "dct/program_generator.h"

namespace DCT {

using namespace DRAMBender;

DRAMBender::Program init_rows(int bank, const std::vector<int>& row_ids, const std::vector<WriteWord_t>& wr_words) {
  struct Reg {
    enum : int{
      NUM_ROWS = ReservedReg::Last,
      ROW_CNT,
      MEM_PTR,       // Base address for stores
      ROW_ADDR,
      Last,
    };
  };

  Program p;

  // Store the row addresses into SoftMC memory
  p.add_inst(SMC_LI(row_ids.size(), Reg::NUM_ROWS));
  p.add_inst(SMC_LI(0, Reg::MEM_PTR));

  // Store the row ID and cacheline into DRAM Bender memory
  // Memory layout: [row addr, 32-bit WR pattern]
  for (size_t i = 0; i < row_ids.size(); i++) {
    p.add_inst(SMC_LI(row_ids[i], Reg::ROW_ADDR));
    p.add_inst(SMC_ST(Reg::MEM_PTR, 0, Reg::ROW_ADDR));
    p.add_inst(SMC_LI(wr_words[i], ReservedReg::PATTERN_REG));
    p.add_inst(SMC_ST(Reg::MEM_PTR, 1, ReservedReg::PATTERN_REG));
    p.add_inst(SMC_ADDI(Reg::MEM_PTR, 2, Reg::MEM_PTR));
  }
  p.add_inst(SMC_LI(0, Reg::MEM_PTR));

  // Load bank and row address registers
  p.add_inst(SMC_LI(bank, ReservedReg::BAR));
  // Column address stride is 8 since we are doing BL=8
  p.add_inst(SMC_LI(8, ReservedReg::CASR));

  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_LI(0, ReservedReg::CAR));

  // Start writing to DRAM rows
  p.add_inst(SMC_LI(0, Reg::ROW_CNT));
  p.add_label("WR_ROW_BEGIN");
  // Load the stored row addresse and data pattern
  p.add_inst(SMC_LD(Reg::MEM_PTR, 0, ReservedReg::RAR));
  p.add_inst(SMC_LD(Reg::MEM_PTR, 1, ReservedReg::PATTERN_REG));
  for (int i = 0; i < 16; i++) {
    p.add_inst(SMC_LDWD(ReservedReg::PATTERN_REG, i));
  }

  p.add_inst(SMC_ACT(ReservedReg::BAR, 0, ReservedReg::RAR, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_LI(0, ReservedReg::CAR));
  p.add_inst(SMC_ADDI(Reg::MEM_PTR, 2, Reg::MEM_PTR));
  for(int i = 0 ; i < 128 ; i++) {
    p.add_inst(SMC_WRITE(ReservedReg::BAR, 0, ReservedReg::CAR, 1, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
    p.add_inst(all_nops()); 
  }
  p.add_inst(SMC_SLEEP(3));
  p.add_inst(SMC_ADDI(Reg::ROW_CNT, 1, Reg::ROW_CNT));

  p.add_inst(SMC_PRE(ReservedReg::BAR, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());

  p.add_branch(p.BR_TYPE::BL, Reg::ROW_CNT, Reg::NUM_ROWS, "WR_ROW_BEGIN");

  p.conclude();
  return p;
}
  
} // namespace DCT
