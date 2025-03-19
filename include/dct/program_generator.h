#ifndef PROGRAM_GENERATOR_H
#define PROGRAM_GENERATOR_H


#include <stdint.h>
#include <vector>

#include "dct/api/program/program.h"


namespace DCT {

using WriteWord_t = uint32_t;   // 512-bit write pottern: 16*32-bit DRAM Bender words
using WritePattern_t = std::array<uint32_t, 16>;   // 512-bit write pottern: 16*32-bit DRAM Bender words

struct ReservedReg {
  enum : int{
    CASR = 0,
    BASR,
    RASR,
    CAR,
    BAR,
    RAR,
    PATTERN_REG,
    Last,
  };
};

DRAMBender::Program init_row(int bank, int row_id, const WritePattern_t& wr_pattern);
DRAMBender::Program init_rows(int bank, const std::vector<int>& row_ids, const std::vector<WriteWord_t>& wr_words);
DRAMBender::Program init_row_range(int bank, int start_row, int end_row, const WritePattern_t& wr_pattern);

DRAMBender::Program read_row(int bank, int row_id);
DRAMBender::Program read_rows(int bank, const std::vector<int>& row_ids);
DRAMBender::Program read_row_range(int bank, int start_row, int end_row);

DRAMBender::Program act_row(int bank, int row_id, int nRAS, int nRP);
DRAMBender::Program act_rows(int bank, const std::vector<int>& row_ids, int nRAS, int nRP);
DRAMBender::Program refresh_all_rows(int bank, int num_rows);

DRAMBender::Program singleside_hammer(int bank, int aggr_id, int hammer_count, int nAggON, int nAggOFF);
DRAMBender::Program doubleside_hammer(int bank, int aggr1_id, int aggr2_id, int hammer_count, int nAggON1, int nAggOFF1, int nAggON2, int nAggOFF2);

DRAMBender::Program half_double();

DRAMBender::Program hammer_pattern(int bank, const std::vector<int>& row_ids, int num_repetitions);

DRAMBender::Program rowclone(int bank, int src_id, int dst_id, int nT1);
DRAMBender::Program tWR_test(int bank, int row_id, int col_id, const WriteWord_t& wr_word, int tWR);
DRAMBender::Program frac(int bank, int row_id, const WriteWord_t& wr_word, int Tfrac1, int Tfrac2, int num_fracs);
// DRAMBender::Program multirow_act(int bank, const Row& rowA, const Row& rowB, int nT1, int nT2);


}

#endif // PROGRAM_GENERATOR_H
