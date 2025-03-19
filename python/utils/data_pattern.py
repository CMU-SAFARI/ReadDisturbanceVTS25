import numpy as np
from typing import Optional, Union

class DataPattern:
  def __init__(self, data_pattern: Union[np.ndarray, int] = 0xDEADBEEF, 
               dq_mapping = None, bitline_mapping = None):
    if isinstance(data_pattern, int):
      self.data_pattern = np.full(16, data_pattern, np.uint32)
    elif isinstance(data_pattern, np.ndarray) and data_pattern.shape == (16,) and data_pattern.dtype == np.uint32:
      self.data_pattern = data_pattern
    else:
      raise ValueError("Invalid data pattern!")  
    
    self.dq_mapping = dq_mapping
    self.bitline_mapping = bitline_mapping

  @property
  def wr_pattern(self) -> int:
    pattern = self.data_pattern
    if self.bitline_mapping is not None:
      pattern = self.bitline_mapping(self._data_pattern)
    if self.dq_mapping is not None:
      pattern = self.dq_mapping(self._data_pattern)
    return pattern

  def __repr__(self) -> str:
    print(f"DataPattern(data_pattern={self.data_pattern}, write_pattern={self.wr_pattern})")
