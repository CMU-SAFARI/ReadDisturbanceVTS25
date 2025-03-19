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
    description='RH HCF',
    formatter_class=argparse.ArgumentDefaultsHelpFormatter
  )

  parser.add_argument('module', 
                      type=str, 
                      help="Module label")
  parser.add_argument('mapping', 
                      type=str, 
                      help="Row Mapping scheme")
  parser.add_argument('--bank',
                      type=int,
                      default=1,
                      help="Bank id to test")

  args = parser.parse_args()
  return args


if __name__ == "__main__":
  args = parse_arguments()
  print(args)
  module = args.module
  mapping = args.mapping
  bank_id = args.bank
  agg_data_patterns = [0x0, 0xFFFFFFFF]
  vic_data_patterns = [0xFFFFFFFF, 0x0]

  # i = SoftMC_Host([module])
  # i.set_module_temperature(module, 50)
  p = DDR4(0, "XDMA")
  p.resetFPGA()
  p.setAREF(False)

  data_prefix = f"../data/"
  os.makedirs(data_prefix, exist_ok=True)

  results = []
  bitflips_record = []
  for itr in range(1):
    for vic_row_id in range(1024, 1024+2048):
      for dp_id in range(2):
        vic_data_pattern = vic_data_patterns[dp_id]
        agg_data_pattern = agg_data_patterns[dp_id]
        vic_row = Row(physical_id=vic_row_id, row_mapping=mapping, data_pattern=vic_data_pattern)
        upper_aggr_row = Row(physical_id=vic_row_id+1, row_mapping=mapping, data_pattern=agg_data_pattern) 
        lower_aggr_row = Row(physical_id=vic_row_id-1, row_mapping=mapping, data_pattern=agg_data_pattern) 

        found = False
        for hc in range(10000, 1000000, 10000):
          if found:
            break
          progs = [
            init_rows(
              bank_id, 
              [vic_row.logical_id, upper_aggr_row.logical_id, lower_aggr_row.logical_id], 
              [vic_row.wr_pattern[0], upper_aggr_row.wr_pattern[0], lower_aggr_row.wr_pattern[0]]
            ),
            singleside_hammer(bank_id, upper_aggr_row.logical_id, hc, 0, 0),
            read_rows(bank_id, [vic_row.logical_id]),
          ]
          p.execute(progs)
          bitflips = get_bitflips(p, [vic_row.wr_pattern])
          for j, bitflip_mask in enumerate(bitflips):
            num_bitflips = popcount_uint64(bitflip_mask)
            if num_bitflips > 0:
              print(f"[ITR {itr}] Victim Row {vic_row_id}, Data Pattern 0x{vic_data_pattern:08X}, HC {hc}, Upper Aggr., Num Bitflips {num_bitflips}")
              results.append([vic_row_id, f"0x{vic_data_pattern:08X}", hc, "Upper", num_bitflips, itr])
              bitflips_record.append([vic_row_id, f"0x{vic_data_pattern:08X}", hc, "Upper", bitflip_mask, itr])
              found = True

        found = False
        for hc in range(10000, 1000000, 10000):
          if found:
            break
          progs = [
            init_rows(
              bank_id, 
              [vic_row.logical_id, upper_aggr_row.logical_id, lower_aggr_row.logical_id], 
              [vic_row.wr_pattern[0], upper_aggr_row.wr_pattern[0], lower_aggr_row.wr_pattern[0]]
            ),
            singleside_hammer(bank_id, lower_aggr_row.logical_id, hc, 0, 0),
            read_rows(bank_id, [vic_row.logical_id]),
          ]
          p.execute(progs)
          bitflips = get_bitflips(p, [vic_row.wr_pattern])
          for j, bitflip_mask in enumerate(bitflips):
            num_bitflips = popcount_uint64(bitflip_mask)
            if num_bitflips > 0:
              print(f"[ITR {itr}] Victim Row {vic_row_id}, Data Pattern 0x{vic_data_pattern:08X}, HC {hc}, Lower Aggr., Num Bitflips {num_bitflips}")
              results.append([vic_row_id, f"0x{vic_data_pattern:08X}", hc, "Lower", num_bitflips, itr])
              bitflips_record.append([vic_row_id, f"0x{vic_data_pattern:08X}", hc, "Lower", bitflip_mask, itr])
              found = True

        found = False
        for hc in range(1000, 500000, 1000):
          if found:
            break
          progs = [
            init_rows(
              bank_id, 
              [vic_row.logical_id, upper_aggr_row.logical_id, lower_aggr_row.logical_id], 
              [vic_row.wr_pattern[0], upper_aggr_row.wr_pattern[0], lower_aggr_row.wr_pattern[0]]
            ),
            doubleside_hammer(bank_id, upper_aggr_row.logical_id, lower_aggr_row.logical_id, hc, 0, 0, 0, 0),
            read_rows(bank_id, [vic_row.logical_id]),
          ]
          p.execute(progs)
          bitflips = get_bitflips(p, [vic_row.wr_pattern])
          for j, bitflip_mask in enumerate(bitflips):
            num_bitflips = popcount_uint64(bitflip_mask)
            if num_bitflips > 0:
              print(f"[ITR {itr}] Victim Row {vic_row_id}, Data Pattern 0x{vic_data_pattern:08X}, HC {hc}, Double Aggr., Num Bitflips {num_bitflips}")
              results.append([vic_row_id, f"0x{vic_data_pattern:08X}", hc, "Double", num_bitflips, itr])
              bitflips_record.append([vic_row_id, f"0x{vic_data_pattern:08X}", hc, "Double", bitflip_mask, itr])
              found = True

  results_df = pd.DataFrame(results, columns=["Vic Row", "Data Pattern", "HC", "Aggr. Type", "Num. Bitflips", "Itr"])
  bitflips_record_df = pd.DataFrame(bitflips_record, columns=["Vic Row", "Data Pattern", "HC", "Aggr. Type", "Bitflips Mask", "Itr"])
  results_df.to_csv(f"{data_prefix}/{module}_rd_hcf.csv", index=False)
  with gzip.open(f"{data_prefix}/{module}_rd_hcf_bitflips.gz", 'wb') as f:
    pickle.dump(bitflips_record_df, f)
  p.close()
