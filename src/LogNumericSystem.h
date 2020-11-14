#ifndef NOWTECH_LOG_NUMERIC_SYSTEM
#define NOWTECH_LOG_NUMERIC_SYSTEM

#include <cstdint>

namespace nowtech::log {

  struct NumericSystem {
    static constexpr uint8_t csInvalid =  0u;
    static constexpr uint8_t csBaseMax = 16u;
  };
  
}

#endif