
#ifndef NOWTECH_LOG_MESSAGE_VARIANT
#define NOWTECH_LOG_MESSAGE_VARIANT

#include "LogMessageBase.h"
#include <array>
#include <variant>
#include <cstddef>
#include <type_traits>


namespace nowtech::log {

// Will be copied using operator=
template<size_t tPayloadSize>
class MessageVariant final : public MessageBase<tPayloadSize> {
public:
  static constexpr size_t csPayloadSize = tPayloadSize;

private:
  using Payload32 = std::variant<bool, float, uint8_t, uint16_t, uint32_t, int8_t, int16_t, int32_t, char, char const*, std::array<char, csPayloadSize>>;
  using Payload64 = std::variant<bool, float, double, uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t, char, char const*, std::array<char, csPayloadSize>>;
  using Payload80 = std::variant<bool, float, double, long double, uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t, char, char const*, std::array<char, csPayloadSize>>;
  using Payload = std::conditional_t<tPayloadSize < sizeof(int64_t) && sizeof(char*) == sizeof(int32_t), Payload32,
                  std::conditional_t<tPayloadSize < sizeof(long double), Payload64, Payload80>>;
  static_assert(std::is_trivially_copyable_v<Payload>);
  // TODO concept on payload size and long double -- see it when 8 in main

  Payload         mPayload;
  LogFormat       mFormat;
  TaskId          mTaskId;
  MessageSequence mMessageSequence;

public:
  MessageVariant() = default;
  MessageVariant(MessageVariant const &) = default;
  MessageVariant(MessageVariant &&) = default;
  MessageVariant& operator=(MessageVariant const &) = default;
  MessageVariant& operator=(MessageVariant &&) = default;

  template<typename tArgument>
  void set(tArgument const aValue, LogFormat const aFormat, TaskId const aTaskId, MessageSequence const aMessageSequence) noexcept {
    mPayload = aValue;
    mFormat = aFormat;
    mTaskId = aTaskId;
    mMessageSequence = aMessageSequence;
  }

  template<typename tConverter>
  void output(tConverter& aConverter) const noexcept {
    auto visitor = [this, &aConverter](const auto aObj) { aConverter.convert(aObj, mFormat.mBase, mFormat.mFill); };
    std::visit(visitor, mPayload);
  }

  bool isTerminal() const noexcept {
    return mMessageSequence == MessageBase<tPayloadSize>::csTerminal;
  }

  uint8_t getBase() const noexcept {
    return mFormat.mBase;
  }  

  uint8_t getFill() const noexcept {
    return mFormat.mFill;
  }  

  TaskId getTaskId() const noexcept {
    return mTaskId;
  }  

  MessageSequence getMessageSequence() const noexcept {
    return mMessageSequence;
  }  
};

}

#endif