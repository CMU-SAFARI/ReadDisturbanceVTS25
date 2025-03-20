# Revisiting DRAM Read Disturbance: Identifying Inconsistencies Between Experimental Characterization and Device-Level Studies

This repository provides the data and code of our [PLACEHOLDER! VTS'25 paper](https://www.google.com): 
> Haocong Luo, İsmail Emir Yüksel, Ataberk Olgun, A. Giray Yağlıkçı, Onur Mutlu, "Revisiting DRAM Read Disturbance: Identifying Inconsistencies Between Experimental Characterization and Device-Level Studies", VTS'25.

In this paper, we attempt to align and cross-validate the real-chip experimental characterization results and state-of-the-art device-level studies of DRAM read disturbance. To do so, we first identify and extract the key bitflip characteristics of RowHammer and RowPress from the device-level error mechanisms studied in prior works. Then, we perform experimental characterization on 96 COTS DDR4 DRAM chips that directly match the data and access patterns studied in the device-level works. Through our experiments, we identify fundamental inconsistencies in the RowHammer and RowPress bitflip directions and access pattern dependence between experimental characterization results and the device-level error mechanisms.

Please use the following citation to cite our study if the repository is useful for you.
```
@inproceedings{luo2025revisiting,
    title={{Revisiting DRAM Read Disturbance: Identifying Inconsistencies Between Experimental Characterization and Device-Level Studies}},
    author={Luo, Haocong and Emir Yüksel, İsmail and Olgun, Ataberk and Yağlıkçı, A. Giray and Mutlu,  Onur},
    year={2025},
    booktitle={{VTS}}
}
```

## Prerequisite
Our real DRAM chip characterization is based on the open-source FPGA-based DRAM characterization infrastructure [DRAM Bender](https://github.com/CMU-SAFARI/DRAM-Bender). Please check out and follow the installation instructions of [DRAM Bender](https://github.com/CMU-SAFARI/DRAM-Bender).

The software dependencies for the characterization are:
- GNU Make, CMake 3.10+
- `c++-17` build toolchain (tested with `gcc-9`)
- Python 3.9+
- `pip` packages `pandas`, `scipy`, `matplotlib`, and `seaborn`

## Example Hardware Setup
Our real DRAM chip characterization infrastructure consists of the following components:
- A host x86 machine with a PCIe 3.0 x16 slot
- An FPGA board with a DIMM/SODIMM slot supported by [DRAM Bender](https://github.com/CMU-SAFARI/DRAM-Bender) (e.g., Xilinx Alveo U200)
- Heater pads attached to the DRAM module under test
- A temperature controller (e.g., MaxWell FT200) programmable by the host machine connected to the heater pads

## Directory Structure
```
data              # Raw data from our experiments
ext               # External libraries
include           # Headers for the C++ DRAM Bender code
python            # High-level Python scripts to conduct the characterization experiments
python-bindings   # Python bindings for the C++ DRAM Bender code
src               # C++ Source code 
  └ api               # Code for interfacing with DRAM Bender hardware
  └ programs          # Low-level DRAM Bender test programs
plots.ipynb       # Jupyter notebook to reproduce the figures and tables in the paper
```

## Running Characterization Experiments

### Step 0
The real DRAM chip characterization takes a long period of time. Therefore, it is recommended to run the characterization experiment script in a persistent shell session (e.g., using a terminal multiplexer like screen, tmux). We also include the raw data we have already collected that can be use to reproduce all the results (figures and tables) in the paper. To do so, go to step 3 directly.

### Step 1 

Clone the repo in your home directory
```
  $ git clone https://github.com/CMU-SAFARI/ReadDisturbanceVTS25
```

Build the C++ interface to the DRAM Bender hardware
```
  $ mkdir build
  $ cd build
  $ cmake ..
  $ make
```

### Step 2 

Go to the python scripts folder
```
  $ cd python
```

Run `run_.sh` to perform all characterization experiments. Characterization data will be produced in the `data/` directory. Make sure to adapt the code to work with your own temperature control hardware!
```
  $ ./run_.sh <your memory module label>
```
### Step 3

Execute all the cells in the Jupyter notebook `plots.ipynb` to reproduce all the figures and tables in the paper.


## Contact:
Haocong Luo (haocong.luo [at] safari [dot] ethz [dot] ch)  
