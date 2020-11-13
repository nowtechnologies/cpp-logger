#ifndef NOWTECH_LOG_MESSAGE_BASE
#define NOWTECH_LOG_MESSAGE_BASE

#include <cstdint>

namespace nowtech::log {

enum class NumericBase : uint8_t {
  cInvalid        =  0u,
  cBinary         =  2u,
  cDecimal        = 10u,
  cHexadecimal    = 16u
};

struct LogFormat final {
public:
  uint8_t mBase;
  uint8_t mFill;

  LogFormat() = default;
  LogFormat(LogFormat const &) = default;
  LogFormat(LogFormat &&) = default;
  LogFormat& operator=(LogFormat const &) = default;
  LogFormat& operator=(LogFormat &&) = default;

  void set(NumericBase const aBase, uint8_t const aFill) noexcept {
    mBase = static_cast<uint8_t>(aBase);
    mFill = aFill;
  }

  uint8_t getBase() const noexcept {
    return mBase;
  }

  uint8_t getFill() const noexcept {
    return mFill;
  }

  void invlidate() noexcept {
    mBase = static_cast<uint8_t>(NumericBase::cInvalid);
  }

  bool isValid() const noexcept {
    auto base = static_cast<uint8_t>(mBase);
    return base == static_cast<uint8_t>(NumericBase::cBinary)
        || base == static_cast<uint8_t>(NumericBase::cDecimal)
        || base == static_cast<uint8_t>(NumericBase::cHexadecimal);
  }
};

using TaskId          = uint8_t;
using MessageSequence = uint8_t;

template<bool tSupport64>
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