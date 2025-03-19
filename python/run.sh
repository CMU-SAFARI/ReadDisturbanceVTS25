#!/bin/bash

echo "Make sure the adapt the scripts for your own temperature control infrastructure!"

module_label=${1}
python3 di_retention.py ${module_label}
python3 di_rd_hc.py ${module_label}
python3 di_rd_ber.py ${module_label}
python3 di_rd_rp.py ${module_label}
python3 di_ds_sweep.py ${module_label}