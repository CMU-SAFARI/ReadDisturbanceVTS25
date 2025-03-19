import numpy as np

POPCOUNT_TABLE = np.array([bin(i).count('1') for i in range(256)], dtype=np.uint8)
def popcount_uint64(arr):
  arr = np.ascontiguousarray(arr, dtype=np.uint64)
  bytes_view = arr.view(np.uint8)
  return POPCOUNT_TABLE[bytes_view].sum()

def get_set_indices(arr):
  arr = np.ascontiguousarray(arr, dtype=np.uint64)
  bytes_view = arr.view(np.uint8)
  bits = np.unpackbits(bytes_view)
  return np.nonzero(bits)[0].tolist()