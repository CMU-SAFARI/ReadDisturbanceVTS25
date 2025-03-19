import numpy as np
from .data_pattern import DataPattern


def type1(addr: int) -> int:
  # Check bit position 3
  return addr ^ (((addr >> 3) & 1) * 0x6)
  

def type2(addr: int) -> int:
  # Check bit positions 3,10,12,14 (mask: 0x5408)
  parity = bin(addr & 0x5408).count('1') & 1   # Use bit_count() in python 3.10+
  return addr ^ (parity * 0x6)


def map_row(addr, mapping):
  if   mapping in ["SA0", "MI0"]:
    return type1(addr)
  elif mapping in ["MI1"]:
    return type2(addr)
  elif mapping in ["Linear", "None", None]:
    return addr


class Row:
  def __init__(self, physical_id: int = -1,  row_mapping = None, 
               data_pattern: DataPattern = 0xDEADBEEF):
    self.physical_id = physical_id
    self.row_mapping = row_mapping
    self.data_pattern = DataPattern(data_pattern)

  @property
  def logical_id(self) -> int:
    return map_row(self.physical_id, self.row_mapping)

  @property
  def wr_pattern(self):
    return self.data_pattern.wr_pattern

  def __repr__(self) -> str:
    return f"Row(physical_id={self.physical_id}, logical_id={self.logical_id}, data_pattern={self.data_pattern})"
    