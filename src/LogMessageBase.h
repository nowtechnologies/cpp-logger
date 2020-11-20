#ifndef NOWTECH_LOG_MESSAGE_BASE
#define NOWTECH_LOG_MESSAGE_BASE

#include "LogNumericSystem.h"

namespace nowtech::log {

struct LogFormat final {
public:
  uint8_t mBase;
  uint8_t mFill;

  void invalidate() noexcept {
    mBase = NumericSystem::csInvalid;
  }

  bool isValid() const noexcept {
    return mBase <= NumericSystem::csBaseMax;
  }
};

using TaskId          = uint8_t;
using MessageSequence = uint8_t;

template<std::size_t tPayloadSize>
class MessageBase {
public:
  static constexpr MessageSequence csTerminal = 0u;

protected:
  static_assert(sizeof(float) <= tPayloadSize);
  static_assert(sizeof(int32_t) <= tPayloadSize);
  static_assert(sizeof(double) <= sizeof(int64_t));
  static_assert(sizeof(char*) <= tPayloadSize);
  static_assert(sizeof(LogFormat) == sizeof(int16_t));
};

} // namespace

#endif