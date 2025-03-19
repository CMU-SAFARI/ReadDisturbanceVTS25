#ifndef DDR4_H
#define DDR4_H

#include "dct/api/platform/platform.h"


namespace DRAMBender {
  

class DDR4 : public IPlatform {
  private:
    const int m_instance_id_;      // instance id to distinguish between the two slots

  public:
    DDR4(int instance_id = 0, const std::string& board = "XDMA");


  // Register my constructor to the base factory
  private:
    static IPlatform* create_(int instance_id) { return new DDR4(instance_id); };
    static bool register_() { IPlatform::registerClass("DDR4", &DDR4::create_); return true; };
    inline static bool autoRegister = register_();
};


} // namespace DRAMBender 

#endif  // DDR4_H