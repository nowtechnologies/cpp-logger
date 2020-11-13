
#ifndef NOWTECH_LOG_MESSAGE_VARIANT
#define NOWTECH_LOG_MESSAGE_VARIANT

#include "LogMessageBase.h"
#include <variant>
#include <type_traits>

namespace nowtech::log {

// Will be copied using operator=
template<bool tSupport64>
class MessageVariant final : public MessageBase<tSupport64> {
  using Payload32 std::variant<float, uint8_t, uint16_t, uint32_t, int8_t, int16_t, int32_t, char, char*>;
  using Payload64 std::variant<float, double, uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t, char, char*>;
  using Payload = std::conditional_t<tSupport64, Payload64, Payload32>;
  static_assert(std::is_trivially_copyable_t<Paylopad>);

  Payload         mPayload;
  LogFormat    mFormat;
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
    auto visitor = [mFormat](const auto aObj) { aConverter.convert(aObj, mFormat.getBase(), mFormat.getFill()); };
    std::visit(visitor, mPayload);
  }

  bool isTerminal() const noexcept {
    return mMessageSequence == csTerminal;
  }

  uint8_t getBase() const noexcept {
    return mFormat.getBase();
  }  

  uint8_t getFill() const noexcept {
    return mFormat.getFill();
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