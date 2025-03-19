#include "dct/program_generator.h"

namespace DCT {

using namespace DRAMBender;

DRAMBender::Program act_row(int bank, const Row& row, int nRAS, int nRP) {
  Program p;
  nb::list access_pattern = nb::cast<nb::list>(pattern.attr("access_pattern"));
  std::vector<nb::object> access_objects;
  for (nb::handle item : access_pattern) {
      nb::object obj = nb::borrow<nb::object>(item);
      access_objects.push_back(obj);  // Adds a reference, managing the object’s lifecycle.
  }
  uint repetitions = nb::cast<uint>(pattern.attr("repetitions"));
  if(access_objects.empty() || repetitions == 0){
    return p;
  }
  int pivot_id = nb::cast<int>(pattern.attr("pivot_id"));
  int head_offset = nb::cast<int>(pattern.attr("head_offset"));
  int tail_offset = nb::cast<int>(pattern.attr("tail_offset"));

  // Read the data from the dram_info object
  int banks = nb::cast<int>(dram_info.attr("banks"));
  int rows = nb::cast<int>(dram_info.attr("rows"));
  nb::object row_mapper = nb::cast<nb::object>(dram_info.attr("row_mapper"));
  // Perform sanity checks
  if(bank < 0 || bank > banks-1){
    throw std::runtime_error("The specified bank is out of bounds.");
  }
  if(pivot_address + head_offset < 0 || pivot_address + tail_offset > rows-1){
    throw std::runtime_error("The pattern doesn't fit into the DRAM address space when mounted to pivot_address.");
  }
  const uint NUM_REPETITIONS_REG = 3;
  const uint REPETITIONS_COUNTER_REG = 4;
  const uint BANK_ADDRESS_REG = 5;
  p.add_inst(SMC_LI(repetitions, NUM_REPETITIONS_REG));
  p.add_inst(SMC_LI(0, REPETITIONS_COUNTER_REG));
  p.add_inst(SMC_LI(bank, BANK_ADDRESS_REG));
  // 10 registers are left for the aggressor rows (6 - 15(included))
  // map rows to registers
  std::map<uint, uint> map_row_id_to_register;
  uint next_free_register = 6;
  for (uint i = 0; i < access_objects.size(); i++){
    uint row_id = nb::cast<uint>(access_objects[i].attr("id"));
    if(map_row_id_to_register.find(row_id) == map_row_id_to_register.end()){
      if (next_free_register > 15){
        throw std::runtime_error("The maximum number of 10 different aggressor rows has been exceeded!");
      }
      map_row_id_to_register.insert(std::pair<uint, uint>(row_id, next_free_register));
      uint physical_row_address = row_id - pivot_id + pivot_address;
      uint logical_row_address = nb::cast<uint>(row_mapper.attr("map_row")(physical_row_address));
      p.add_inst(SMC_LI(logical_row_address, next_free_register));
      next_free_register += 1;
    }
  }

  // The following three definitions are referring to the last access object
  uint last_row_id = nb::cast<uint>(access_objects.back().attr("id")); 
  uint last_on = nb::cast<int>(access_objects.back().attr("on"));
  uint last_off = nb::cast<int>(access_objects.back().attr("off"));
  
  // Treat the first repetition seperately
  p.add_inst(SMC_PRE(BANK_ADDRESS_REG, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_ADDI(REPETITIONS_COUNTER_REG, 1, REPETITIONS_COUNTER_REG));  // This takes one cycle 
  for(uint i = 0; i < access_objects.size()-1; i++){
    uint row_id = nb::cast<uint>(access_objects[i].attr("id"));
    uint on = nb::cast<int>(access_objects[i].attr("on"));
    uint off = nb::cast<int>(access_objects[i].attr("off"));
    if(on < 5){
      throw std::runtime_error("A minimum of 5 on cycles are required for each row. (t_RAS = 33ns)");
    }
    if(off < 2){
      throw std::runtime_error("A minimum of 2 on cycles are required for each row. (t_RP = 12ns)");
    }
    p.add_inst(SMC_NOP(), SMC_ACT(BANK_ADDRESS_REG, 0, map_row_id_to_register[row_id], 0), SMC_NOP(), SMC_NOP());
    p.add_inst(SMC_SLEEP(on)); // We wait for 3ns too long here!
    p.add_inst(SMC_PRE(BANK_ADDRESS_REG, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());

    // This wait time is accurate
    if(off < 4){
      for (uint j = 0; j < off-1; j++){
        p.add_inst(all_nops());
      }
    }
    else {
      p.add_inst(SMC_SLEEP(off-1));
    }
  }
  // We treat the last row separately here
  p.add_inst(SMC_NOP(), SMC_ACT(BANK_ADDRESS_REG, 0, map_row_id_to_register[last_row_id], 0), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_SLEEP(last_on));  // We sleep for 3ns too long here

  if(repetitions > 1){
    ////////////////////////////////////////////////////////////////// Here we treat the rest of the repetitions
    p.add_label("HMR_BEGIN");
    if(last_on < 5){
      throw std::runtime_error("A minimum of 5 on cycles are required for each row. (t_RAS = 33ns)");
    }
    if(last_off < 2){
      throw std::runtime_error("A minimum of 2 off cycles are required for each row. (t_RP = 12ns)");
    }
    p.add_inst(SMC_PRE(BANK_ADDRESS_REG, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
    p.add_inst(SMC_ADDI(REPETITIONS_COUNTER_REG, 1, REPETITIONS_COUNTER_REG));  // This takes one cycle 

    // the wait time is exact here
    if(last_off < 5){   
      // last_off < 5 implies that we need to sleep for less than 3 cycles
      for (uint j = 0; j < last_off - 2; j++){
        p.add_inst(all_nops());
      }
    }
    else {
      p.add_inst(SMC_SLEEP(last_off-2));
    }

    for(uint i = 0; i < access_objects.size()-1; i++){
      uint row_id = nb::cast<uint>(access_objects[i].attr("id"));
      uint on = nb::cast<int>(access_objects[i].attr("on"));
      uint off = nb::cast<int>(access_objects[i].attr("off"));
      if(on < 5){
        throw std::runtime_error("A minimum of 6 on cycles are required for each row. (t_RAS = 33ns)");
      }
      if(off < 2){
        throw std::runtime_error("A minimum of 2 on cycles are required for each row. (t_RP = 12ns)");
      }
      p.add_inst(SMC_NOP(), SMC_ACT(BANK_ADDRESS_REG, 0, map_row_id_to_register[row_id], 0), SMC_NOP(), SMC_NOP());
      p.add_inst(SMC_SLEEP(on)); // We wait for 3ns too long here!
      p.add_inst(SMC_PRE(BANK_ADDRESS_REG, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());

      // This wait time is accurate
      if(off < 4){
        for (uint j = 0; j < off-1; j++){
          p.add_inst(all_nops());
        }
      }
      else {
        p.add_inst(SMC_SLEEP(off-1));
      }

    }

    // We treat the last row separately here
    p.add_inst(SMC_NOP(), SMC_ACT(BANK_ADDRESS_REG, 0, map_row_id_to_register[last_row_id], 0), SMC_NOP(), SMC_NOP());

    // we wait for 3ns too long here!
    if (last_on < 9){
      for (uint j = 0; j < last_on - 6; j++){
        p.add_inst(all_nops());
      }
    }
    else{
      p.add_inst(SMC_SLEEP(last_on - 6));
    }
    p.add_branch(p.BR_TYPE::BL, REPETITIONS_COUNTER_REG, NUM_REPETITIONS_REG, "HMR_BEGIN"); // This takes 6 cycles
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
  }
  p.add_inst(SMC_PRE(BANK_ADDRESS_REG, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_END());

  return p;
}
  
} // namespace DCT
