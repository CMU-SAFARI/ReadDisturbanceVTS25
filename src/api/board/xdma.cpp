#include <string>
#include <fstream>
#include <iostream>

#include <unistd.h>
#include <fcntl.h>
#include <cstring> 
#include <cassert>

#include "dct/api/board/board.h"


namespace DRAMBender {

/**
 * @brief XDMA board interface implemenation
 * 
 */
class XDMA : public IBoard {
  private:
    const int instance_id_;                     // The ID of the DRAM Bender instance (corresponds to different DIMM slot)

    void* m_send_buf_;                          // send buffer 
    const size_t send_buffer_size_;             // send buffer size in bytes (Note that XDMA AXI datapath width is 256 bits, i.e., 32 bytes)

    void* m_recv_buf_;                          // receive buffer
    const size_t recv_buffer_size_;             // receive buffer size in bytes

    int m_to_card_fd_;                          // File descriptor for sending data to FPGA
    const std::string to_FPGA_prefix   = "/dev/xdma0_h2c_";   // Default device path for sending data to FPGA

    int m_from_card_fd_;                        // File descriptor for receiving data from FPGA
    const std::string from_FPGA_prefix = "/dev/xdma0_c2h_";   // Default device path for receiving data from FPGA

  public:
    XDMA(int instance_id = 0, int send_buffer_size = 32 * 2048, int recv_buffer_size = 32 * 1024): 
    instance_id_(instance_id), send_buffer_size_(send_buffer_size), recv_buffer_size_(recv_buffer_size) {};

    ~XDMA() {
      free(m_send_buf_);
      free(m_recv_buf_);
      close(m_to_card_fd_);
      close(m_from_card_fd_);
    };

    int init() override {
      // Try to open the to_fpga file
      std::string to_fpga_file = to_FPGA_prefix + std::to_string(instance_id_);
      m_to_card_fd_ = open(to_fpga_file.c_str(), O_RDWR);
      if (m_to_card_fd_ < 0) {
        std::cerr << "Open to card (" << to_fpga_file << ") failed!" << std::endl;
        return 1;
      } else {
        std::cout << "Opened " << to_fpga_file <<  " -> fd: " << m_to_card_fd_ << std::endl;
      }

      // Try to open the from_fpga file
      std::string from_fpga_file = from_FPGA_prefix + std::to_string(instance_id_);
      m_from_card_fd_ = open(from_fpga_file.c_str(), O_RDWR);
      if (m_from_card_fd_ < 0) {
        std::cerr << "Open to card (" << from_fpga_file << ") failed!" << std::endl;
        return 1;
      } else {
        std::cout << "Opened " << from_fpga_file <<  " -> fd: " << m_from_card_fd_ << std::endl;
      }

      // allocate page size aligned X page size regions to our buffers
      if (posix_memalign(&m_send_buf_, 4096, send_buffer_size_ + (4096-(send_buffer_size_ % 4096)))) {
        std::cerr << "Send buffer allocation failed!" << std::endl;
        return 2;
      }
      if (posix_memalign(&m_recv_buf_, 4096, recv_buffer_size_ + (4096-(recv_buffer_size_ % 4096)))) {
        std::cerr << "Receive buffer allocation failed!" << std::endl;
        return 2;
      }
      if ((!m_send_buf_) || (!m_recv_buf_)) {
        std::cerr << "Buffers cannot be allocated!" << std::endl;
        return 2;
      }

      return 0;
    };
    
    size_t sendData(void* data, const size_t size) override {
      memcpy((char*)m_send_buf_, (char*)data, size);

      size_t count = 0;
    
      while (count < size) {
        ssize_t rc = write(m_to_card_fd_, (char*)m_send_buf_, size);
        assert(rc == (ssize_t)size || rc == 0);
        count += rc;
      }

      return count;
    };

    std::tuple<const void*, size_t> recvData(const size_t size) override {
      assert(size <= recv_buffer_size_ && "given read size is too large");

      int fd = m_from_card_fd_;
      size_t count = 0;
      count = read(fd, (char*) m_recv_buf_, size);
      assert(count <= size);
      return {m_recv_buf_, count};
    };

  // Register my constructor to the base factory
  private:
    static XDMA* create_(int instance_id) { return new XDMA(instance_id); };
    static bool register_() { IBoard::registerClass("XDMA", &XDMA::create_); return true; };
    inline static bool autoRegister = register_();
};


}   // namespace DRAMBender


