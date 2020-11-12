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

class LogFormatEnd final {
private:
  static constexpr uint8_t csBaseMask = 127u;
  static constexpr uint8_t csEndMask  = 128u;

  uint8_t mBaseEnd;    // TODO find out if these should have default values
  uint8_t mFill;

public:
  LogFormatEnd() = default;
  LogFormatEnd(LogFormatEnd const &) = default;
  LogFormatEnd(LogFormatEnd &&) = default;
  LogFormatEnd& operator=(LogFormatEnd const &) = default;
  LogFormatEnd& operator=(LogFormatEnd &&) = default;

  void set(NumericBase const aBase, bool const aEnd, uint8_t const aFill) noexcept {
    mBaseEnd = static_cast<uint8_t>(aBase) | (aEnd ? csEndMask : 0u);
    mFill = aFill;
  }

  void setEnd(bool const aEnd) noexcept {
    mBaseEnd |= (aEnd ? csEndMask : 0u);
  }

  uint8_t getBaseEnd() const noexcept {
    return mBaseEnd;
  }

  NumericBase getBase() const noexcept {
    return static_cast<NumericBase>(mBaseEnd & csBaseMask);
  }

  static NumericBase uint2base(uint8_t const aBaseEnd) noexcept {
    return static_cast<NumericBase>(aBaseEnd & csBaseMask);
  }

  bool isTerminal() const noexcept {
    return (mBaseEnd & csEndMask) != 0u;
  }

  static bool uint2terminal(uint8_t const aBaseEnd) noexcept {
    return (aBaseEnd & csEndMask) != 0u;
  }

  uint8_t getFill() const noexcept {
    return mFill;
  }

  void invlidate() noexcept {
    mBaseEnd = static_cast<uint8_t>(NumericBase::cInvalid);
  }

  bool isValid() const noexcept {
    auto base = static_cast<uint8_t>(mBaseEnd) & csBaseMask;
    return base == static_cast<uint8_t>(NumericBase::cBinary)
        || base == static_cast<uint8_t>(NumericBase::cDecimal)
        || base == static_cast<uint8_t>(NumericBase::cHexadecimal);
  }
};

using TaskId          = uint8_t;
using MessageSequence = uint8_t;

/// A message may be terminal if contained LogFormatEnd.isTerminal or isTerminal()
template<bool tSupport64>
class MessageBase {
protected:
  using LargestPayload = std::conditional_t<tSupport64, int64_t, int32_t>;

  static_assert(sizeof(float) <= sizeof(LargestPayload));
  static_assert(tSupport64 && sizeof(double) <= sizeof(LargestPayload));
  static_assert(sizeof(char*) <= sizeof(LargestPayload));
  static_assert(sizeof(LogFormatEnd) == sizeof(int16_t));
};

} // namespace

#endif