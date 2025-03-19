#ifndef PLATFORM_H
#define PLATFORM_H

#include <string>
#include <map>
#include <thread>

#include <memory.h>

#include <boost/lockfree/spsc_queue.hpp>

#include "dct/api/board/board.h"
#include "dct/api/program/program.h"


namespace DRAMBender {


using Word_t = uint32_t;    // 32-bit word size (e.g., register width) in DRAM Bender
using Inst_t = uint64_t;    // 64-bit instruction length

const size_t axi_datapath_byte_width = 32;  // 256-bit (32 bytes) AXI datapath width 

/**
 * @brief The platform interfaces with the DRAM Bender hardware, providing high-level APIs to execute programs, receive data, reset the logic, set auto refresh, etc.
 * 
 */
class IPlatform {
  protected:
    IBoard* m_board_;                       // A pointer to the board interface

    const int max_num_insts_per_prog_;      // Maximum number of instructions per program
    void* m_send_buffer_;                   // Send buffer: For padding data to fit the 256-bit AXI datapath width
    const size_t send_buffer_size_;         // Send buffer size (in bytes)

    const int readback_buffer_size_;        // The size of the readback buffer on DRAM Bender (in num of AXI transactions)
    using RecvRingBuffer_t = boost::lockfree::spsc_queue<Word_t>;
    RecvRingBuffer_t m_recv_ringbuffer_;    // A ringbuffer that keeps receiving (consuming) data from the board
    std::thread m_receiver_thread_;         // A another thread that receives (consume) the data from board and puts them in the ring buffer

  public:
    IPlatform(int max_num_insts_per_prog=2048, size_t recv_ringbuffer_size=1024*1024*2, int readback_buffer_size=1024):
    max_num_insts_per_prog_(max_num_insts_per_prog), send_buffer_size_(axi_datapath_byte_width*max_num_insts_per_prog),
    readback_buffer_size_(readback_buffer_size), m_recv_ringbuffer_(recv_ringbuffer_size) {
      m_send_buffer_ = malloc(send_buffer_size_);
      memset(m_send_buffer_, 0, send_buffer_size_);
    };

    ~IPlatform() {
      free(m_send_buffer_);
    };

    /**
     * @brief    Closes the platform (joins the receiver thread, for python bindings)
     * 
     */
    void close();

    /**
     * @brief    Sends the DRAM Bender program to the FPGA and execute it
     * 
     */
    virtual void execute(Program& prog);


    /**
     * @brief    Executes all the programs in the queue back to back (does NOT clear the queue)
     * 
     */
    void execute(std::vector<Program>& prog_queue);

    /**
     * @brief    Receive data from the FPGA board over PCI-E
     * 
     * @param dst_buf     Pointer to the buffer that will receive the data
     * @param num_bytes   Number of bytes to read
     * 
     * @return  size_t    Number of bytes read
     * 
     */
    virtual size_t receiveData(void* dst_buf, size_t num_bytes);

    /**
     * @brief    Resets tje DRAM Bender logic, won't reset PCI-E endpoint or the PHY interface
     */
    virtual void resetFPGA();

    /**
     * @brief    Resets everything (including flushing the PCI-E endpoint)
     */
    virtual void fullReset();

    /**
     * @brief    Set AutoRefresh ON/OFF
     * 
     */
    virtual void setAREF(bool is_on);

    /**
     * @brief    Used along with Program::dumpRegisters to read register content
     * 
     */
    virtual void readRegisterDump();


  private:
    void consumeData_();

  // Factory methods to create different board implementations based on a registry
  public:
    template <typename... Args>
    static IPlatform* create(const std::string& derived_name, Args... args) {
      if (auto it = m_registry_.find(derived_name); it != m_registry_.end()) {
        typedef IPlatform* (*CreatedDerived_t)(Args...);
        return reinterpret_cast<CreatedDerived_t>(it->second)(args...);
      }
      return nullptr;
    }

    template<typename CreatedDerived_t>
    static void registerClass(const std::string& derived_name, CreatedDerived_t create_func) {
      m_registry_[derived_name] = reinterpret_cast<void*>(create_func);
    }

  private:
    inline static std::map<std::string, void*> m_registry_;  
};


}   // namespace DRAMBender

#endif // PLATFORM_H
