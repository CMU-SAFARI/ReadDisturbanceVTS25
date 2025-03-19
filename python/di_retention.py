import signal
import time
import os
import argparse
import pickle
import gzip
import subprocess

from utils.row import Row
from utils.bitutils import popcount_uint64
from utils.utils import *

import numpy as np
import pandas as pd

import sys
sys.path.append('../build/')
from pyDRAMBender import *

# from smc_scripts import SoftMC_Host


def parse_arguments():
  parser = argparse.ArgumentParser(
    description='Script to test retention failure behavior of DRAM',
    formatter_class=argparse.ArgumentDefaultsHelpFormatter
  )

  parser.add_argument('module', 
                      type=str, 
                      help="Module label")
  parser.add_argument('--rows',
                      type=int,
                      default=2048,
                      help="Number of rows to test")
  parser.add_argument('--start_offset',
                      type=int, 
                      default=1024, 
                      help="Offset of the start row")
  parser.add_argument('--bank',
                      type=int,
                      default=1,
                      help="Bank id to test")
  parser.add_argument('--t_wait_list',
                      type=int,
                      nargs='+',
                      default=[4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096],
                      help='List of filters to apply')
  parser.add_argument('--temp_list',
                      type=int,
                      nargs='+',
                      default=[90],
                      help='List of filters to apply')
  args = parser.parse_args()
  return args


if __name__ == "__main__":
  args = parse_arguments()
  print(args)
  module = args.module
  num_rows_to_test = args.rows
  row_start_offset = args.start_offset
  bank_id = args.bank
  t_wait_list = args.t_wait_list
  temp_list = args.temp_list
  data_patterns = [0x0, 0xFFFFFFFF]

  # i = SoftMC_Host([module])
  p = DDR4(0, "XDMA")
  p.resetFPGA()
  p.setAREF(False)

  data_prefix = f"../data/"
  os.makedirs(data_prefix, exist_ok=True)

  results = []
  bitflip_records = {}
  for temp in temp_list:
    bitflip_records[temp] = {}
    # i.set_module_temperature(module, temp)
    # SET TEMPERATURE HERE!!!
    for data_pattern in data_patterns:
      bitflip_records[temp][f"{data_pattern:08X}"] = {}
      for t_wait in t_wait_list:
        bitflip_records[temp][f"{data_pattern:08X}"][t_wait] = {}
        progs = [
          init_row_range(bank_id, 0 + row_start_offset, num_rows_to_test + row_start_offset, [data_pattern] * 16),
          refresh_all_rows(bank_id, 32768),
        ]
        p.execute(progs)

        sleep_with_progress(t_wait)

        p.execute(read_row_range(bank_id, 0 + row_start_offset, num_rows_to_test + row_start_offset))

        t = get_bitflips(p, [[data_pattern] * 16] * num_rows_to_test)
        for row_id, bitflip_mask in enumerate(t):
          if np.count_nonzero(bitflip_mask) != 0:
            num_bitflips = popcount_uint64(bitflip_mask)
            print(f"Temp {temp}C, Data Pattern 0x{data_pattern:08X}, tWAIT {t_wait}s, Row {row_id}, Num Bitflips {num_bitflips}")
            results.append([temp, f"{data_pattern:08X}", t_wait, row_id, num_bitflips])
            bitflip_records[temp][f"{data_pattern:08X}"][t_wait][row_id] = bitflip_mask

  df = pd.DataFrame(results, columns=['Temp', "Pattern", "tWAIT", "Row", "NumBitflips"])
  df.to_csv(f"{data_prefix}/{module}_retention.csv", index=False)
  with gzip.open(f"{data_prefix}/{module}_retention_bitflips.gz", 'wb') as f:
    pickle.dump(bitflip_records, f)
  p.close()
  # i.set_module_temperature(module, 50)
 