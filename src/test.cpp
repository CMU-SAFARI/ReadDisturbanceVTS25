#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <random> 
#include <bit> 
#include <immintrin.h>

#include "dct/api/platform/platform.h"
#include "dct/program_generator.h"

using namespace DRAMBender;

#define NUM_BANKS 16
#define NUM_ROWS 1024*32

#define NUM_ROWS_REG  8
#define NUM_BANKS_REG 11
// Stride register ids are fixed and should not be changed
// CASR should always be reg 0
// BASR should always be reg 1
// RASR should always be reg 2
#define CASR 0
#define BASR 1
#define RASR 2

#define PATTERN_REG 12

#define BAR 7
#define RAR 6
#define CAR 4

#define NUM_COLS_REG 14
#define LOOP_COLS 13

#define TEMP_PATTERN_REG 15


int map_row(int addr) {
  return addr ^ (((addr >> 3) & 1) * 0x6);
};

int main() {
  IPlatform* p = IPlatform::create("DDR4", 0);
  p->resetFPGA();

  DCT::WritePattern_t cl;
  cl.fill(0x0);

  DCT::WritePattern_t aggr_cl;
  aggr_cl.fill(0xFFFFFFFF);
  for (int aggr_id = 0; aggr_id < 32; aggr_id++) {
    std::vector<Program> hammer_progs;
    for (int i = 0; i < 8; i++) {
      hammer_progs.push_back(DCT::init_row_range(1, i*8192, (i+1)*8192, cl));
      hammer_progs.push_back(DCT::refresh_all_rows(1, 65536));
    }

    hammer_progs.push_back(DCT::init_row(1, map_row(aggr_id), aggr_cl));
    hammer_progs.push_back(DCT::singleside_hammer(1, map_row(aggr_id), 1000000, 0, 0));
    p->execute(hammer_progs);

    Program read_all = DCT::read_row_range(1, 0, 65536);
    p->execute(read_all);
}