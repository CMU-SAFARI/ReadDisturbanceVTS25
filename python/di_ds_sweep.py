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
    description='Test 0->1 1->0 HC',
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
  p = DDR4(0, "XDMA")
  p.resetFPGA()
  p.setAREF(False)

  data_prefix = f"../data/"
  os.makedirs(data_prefix, exist_ok=True)

  hcf_data = pd.read_csv(f"{data_prefix}/{module}_ds_hcf.csv", index_col=False)
  min_hcf_df = hcf_data.groupby(['Temp', 'Vic Row']).min().reset_index()[['Temp', 'Vic Row', "HC"]]

  # Handle Micron module cell layout
  if module == "axmicr02":
    ret_df = pd.read_csv("../data/axmicr02_retention.csv")
    ret_df = ret_df.loc[ret_df["tWAIT"] == 4096]

    anti_df = ret_df.loc[(ret_df["Pattern"] == "00000000") & (ret_df["NumBitflips"] >= 10)]
    true_df = ret_df.loc[(ret_df["Pattern"] == "FFFFFFFF") & (ret_df["NumBitflips"] >= 10)]

    anti_rows = anti_df["Row"].unique() + 1024

  results = []
  bitflips_record = []
  for temp in [50]:
    # i.set_module_temperature(module, temp)
    # SET TEMPERATURE HERE!!!
    for itr in range(1):
      for vic_row_id in range(1024, 1024+2048):
        
        bers = [None, None]
        mask = (min_hcf_df['Vic Row'] == vic_row_id) & (min_hcf_df['Temp'] == temp)
        hc_min = min_hcf_df.loc[mask, 'HC'].values[0]
        hc_list = range(hc_min, 500000, 1000)

        for hc in hc_list:
          for dp_id in range(2):
            vic_data_pattern = vic_data_patterns[dp_id]
            agg_data_pattern = agg_data_patterns[dp_id]
            vic_row = Row(physical_id=vic_row_id, row_mapping=mapping, data_pattern=vic_data_pattern)
            upper_aggr_row = Row(physical_id=vic_row_id+1, row_mapping=mapping, data_pattern=agg_data_pattern) 
            lower_aggr_row = Row(physical_id=vic_row_id-1, row_mapping=mapping, data_pattern=agg_data_pattern) 
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
                bers[dp_id] = num_bitflips
                print(f"[TEMP {temp} ITR {itr}] Victim Row {vic_row_id}, Data Pattern 0x{vic_data_pattern:08X}, HC {hc}, Double Aggr., Num Bitflips {num_bitflips}")
          if bers[0] is not None and  bers[1] is not None:
            if module == "axmicr02" and vic_row_id in anti_rows:
              if bers[1] >= bers[0]:
                print(f"    [TEMP {temp} ITR {itr}] Victim Row {vic_row_id}, HC {hc}")
                results.append([temp, vic_row_id, hc, itr])
                bitflips_record.append([temp, vic_row_id, hc, itr])
                break
            else:
              if bers[0] >= bers[1]:
                print(f"    [TEMP {temp} ITR {itr}] Victim Row {vic_row_id}, HC {hc}")
                results.append([temp, vic_row_id, hc, itr])
                bitflips_record.append([temp, vic_row_id, hc, itr])
                break

  results_df = pd.DataFrame(results, columns=["Temp", "Vic Row", "HC", "Itr"])
  bitflips_record_df = pd.DataFrame(bitflips_record, columns=["Temp", "Vic Row", "HC", "Itr"])
  results_df.to_csv(f"{data_prefix}/{module}_ds_ber_sweep.csv", index=False)
  with gzip.open(f"{data_prefix}/{module}_ds_ber_sweep_bitflips.gz", 'wb') as f:
    pickle.dump(bitflips_record_df, f)
  p.close()
