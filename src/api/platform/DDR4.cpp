#include <stdexcept>

#include "dct/api/platform/DDR4.h"


namespace DRAMBender {
  

DDR4::DDR4(int instance_id, const std::string& board):
m_instance_id_(instance_id) {
  m_board_ = IBoard::create("XDMA", m_instance_id_);
  int err = m_board_->init();
  if (err) {
    throw std::runtime_error("Failed to initialize board");
  }
};



} // namespace DRAMBender
