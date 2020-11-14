#ifndef NOWTECH_LOG_MESSAGE_BASE
#define NOWTECH_LOG_MESSAGE_BASE

#include "LogNumericSystem.h"

namespace nowtech::log {

struct LogFormat final {
public:
  uint8_t mBase;
  uint8_t mFill;

  LogFormat() = default;
  LogFormat(LogFormat const &) = default;
  LogFormat(LogFormat &&) = default;
  LogFormat& operator=(LogFormat const &) = default;
  LogFormat& operator=(LogFormat &&) = default;

  uint8_t getBase() const noexcept {
    return mBase;
  }

  uint8_t getFill() const noexcept {
    return mFill;
  }

  void invlidate() noexcept {
    mBase = NumericSystem::csInvalid;
  }

  bool isValid() const noexcept {
    return mBase <= NumericSystem::csBaseMax;
  }
};

using TaskId          = uint8_t;
using MessageSequence = uint8_t;

template<typename tSupport64>
class MessageBase {
public:
  static constexpr MessageSequence csTerminal = 0u;

protected:
  using LargestPayload = std::conditional_t<tSupport64, int64_t, int32_t>;

  static_assert(sizeof(float) <= sizeof(LargestPayload));
  static_assert(tSupport64 && sizeof(double) <= sizeof(LargestPayload));
  static_assert(sizeof(char*) <= sizeof(LargestPayload));
  static_assert(sizeof(LogFormat) == sizeof(int16_t));
};

} // namespace

#endif