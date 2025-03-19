#include <iostream>

#include "dct/api/platform/platform.h"
#include "dct/api/board/board.h"


namespace DRAMBender {

void IPlatform::execute(std::vector<Program>& prog_queue) {
  for (Program& prog : prog_queue) {
    execute(prog);
  }
};

void IPlatform::execute(Program& prog) {
  bool is_prog_concluded = prog.is_concluded();
  assert(is_prog_concluded && "Program is not concluded!");

  int num_insts = prog.get_num_insts();
  assert (num_insts <= max_num_insts_per_prog_ && " Number of instructions exceeds the maximum amount.");

  // Zero-pad each instruction to align with the AXI datapath width.
  Inst_t* iseq     = prog.get_inst_seq();
  int stride  = axi_datapath_byte_width / sizeof(Inst_t);
  size_t send_size = num_insts * sizeof(Inst_t) * stride;
  for(int i = 0 ; i < num_insts ; i++) {
    static_cast<Inst_t*>(m_send_buffer_)[i*stride] = iseq[i];
  }
  
  // Joins the receiver thread to make sure data transfer from previous program is finished before starting the next one
  if(m_receiver_thread_.joinable()) {
    m_receiver_thread_.join();
  }
  m_receiver_thread_ = std::thread(&IPlatform::consumeData_, this);

  int sent = m_board_->sendData(m_send_buffer_, send_size /*in bytes*/);
  memset(m_send_buffer_, 0, send_size);

  assert(sent && "could not send instructions");
};


void IPlatform::close() {
  if(m_receiver_thread_.joinable()) {
    m_receiver_thread_.join();
  } else {
    std::cout << "Unable to join the receiver thread!" << std::endl;
  }
}


size_t IPlatform::receiveData(void* dst_buf, size_t num_bytes) {
  int num_words_to_read = num_bytes / sizeof(Word_t);
  int read_word_count = m_recv_ringbuffer_.pop((Word_t*) dst_buf, num_words_to_read);
  while (read_word_count < num_words_to_read) {
    int remaining_words_to_read = num_words_to_read - read_word_count;
    read_word_count += m_recv_ringbuffer_.pop(((Word_t*) dst_buf) + read_word_count, remaining_words_to_read);
  }

  assert(read_word_count == num_words_to_read && "Unexpected amount of data popped from spsc\n");
  return read_word_count * sizeof(Word_t);
};

void IPlatform::consumeData_() {
  while (true) {
    size_t bytes_to_recv = axi_datapath_byte_width * readback_buffer_size_;   // Try to readback 32KB of data
    auto [recv_buffer, recv_count] = m_board_->recvData(bytes_to_recv);
    // printf("[Consume Data] Received %ld \n", recv_count);
    if (recv_count == 0) {
      // Read back all the data
      break;
    }

    int num_words_to_read = bytes_to_recv / sizeof(Word_t);
    if (recv_count != bytes_to_recv) {
    // If we did not read 32KBs then the program probably ended
    // and the last transfer is trash so we discard it
    // DRAM Bender sends an extra 32B data to indicate that a program has ended
      num_words_to_read = (recv_count - axi_datapath_byte_width) / sizeof(Word_t);
    }

    int read_word_count = m_recv_ringbuffer_.push((Word_t*) recv_buffer, num_words_to_read);
    while (read_word_count < num_words_to_read) {
      int remaining_words_to_read = num_words_to_read - read_word_count;
      read_word_count += m_recv_ringbuffer_.push(((Word_t*) recv_buffer) + read_word_count, remaining_words_to_read);
    }
    // printf("[Consume Data] Ringbuffer read %ld \n", read_word_count);
    assert(read_word_count == num_words_to_read && "Unexpected amount of data pushed from spsc\n");
    if(recv_count != bytes_to_recv) {
      break;
    }
  }
};

void IPlatform::resetFPGA() {
  memset(m_send_buffer_, 0, send_buffer_size_);
  ((uint8_t*) m_send_buffer_)[8] = (uint8_t) 1;

  // Just need to send one AXI tx
  size_t byte_sent = m_board_->sendData(m_send_buffer_, axi_datapath_byte_width);
  memset(m_send_buffer_, 0, send_buffer_size_);

  if (byte_sent == axi_datapath_byte_width) {
    std::cout << "Successfully reset the FPGA!" << std::endl;
  } else {
    std::cerr << "Could not reset the FPGA!" << std::endl;
  }
};

void IPlatform::fullReset() {
  // [TODO]: Implement the full reset. Using another thread?
  throw std::logic_error("Function not yet implemented");
};

void IPlatform::setAREF(bool is_on) {
  memset(m_send_buffer_, 0, send_buffer_size_);
  ((uint8_t*) m_send_buffer_)[8] = (uint8_t) 0x8;
  ((uint8_t*) m_send_buffer_)[0] = is_on;

  // Just need to send one AXI tx
  size_t byte_sent = m_board_->sendData(m_send_buffer_, axi_datapath_byte_width);
  memset(m_send_buffer_, 0, send_buffer_size_);
  
  if (byte_sent != axi_datapath_byte_width) {
    std::cerr << "Could not toggle auto refresh!" << std::endl;
  } else {
    std::cout << "Autorefresh: " << is_on << std::endl;
  }
};

void IPlatform::readRegisterDump() {
  // [TODO]: Make this part more elegant?
  /** The author decided to use printfs explicitly within this function
   * because printing unsigned hex bytes is uglier with stdio
   */
  uint8_t readData[64];
  receiveData((void*)readData, 64); // first read wdata content
  printf("WDATA: 0x");
  for(int i = 63 ; i >= 0 ; i--) {
    printf("%x", readData[i]);
  }
  printf("\n");
  receiveData((void*)readData, 64); // read register content
  for(int r = 0 ; r < 16 ; r++) {
    printf("R%d: 0x", r);
    printf("%x", ((uint32_t*)readData)[r]);
    printf("\n");
  }
};

}   // namespace DRAMBender


