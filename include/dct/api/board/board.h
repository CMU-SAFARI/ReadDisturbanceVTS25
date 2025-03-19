#ifndef BOARD_H
#define BOARD_H

#include <string>
#include <tuple>
#include <map>
#include <functional>


namespace DRAMBender {

/**
 * @brief Interface class for low-level communication with the FPGA board
 * 
 */
class IBoard {
  public:
    /**
     * @brief Initializes the board interface
     * 
     * @return int 
     */
    virtual int init() = 0;

    /**
     * @brief Sends data to the FPGA
     * 
     * @param data          Pointer to the data buffer
     * @param size          The number of bytes to send
     * @return size_t       The number of bytes sent
     */
    virtual size_t sendData(void* data, const size_t size) = 0;

    /**
     * @brief    Receives data from the FPGA
     * 
     * @param    size          The number of bytes to receive
     * @return   std::tuple<const void*, size_t>
     *                         A tuple of <the pointer to the receive buffer, the number of bytes actually received>
     */
    virtual std::tuple<const void*, size_t> recvData(const size_t size) = 0;
  

  // Factory methods to create different board implementations based on a registry
  public:
    template <typename... Args>
    static IBoard* create(const std::string& derived_name, Args... args) {
      if (auto it = m_registry_.find(derived_name); it != m_registry_.end()) {
        typedef IBoard* (*CreatedDerived_t)(Args...);
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

#endif // BOARD_H
