
#ifndef NOWTECH_LOG_MESSAGE_VARIANT
#define NOWTECH_LOG_MESSAGE_VARIANT

#include "LogMessageBase.h"
#include <variant>
#include <type_traits>

namespace nowtech::log {

// Will be copied using operator=
template<bool tSupport64>
class MessageVariant final : public MessageBase<tSupport64> {
  using Payload32 std::variant<float, uint8_t, uint16_t, uint32_t, int8_t, int16_t, int32_t, char*>;
  using Payload64 std::variant<float, double, uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t, char*>;
  using Payload = std::conditional_t<tSupport64, Payload64, Payload32>;
  static_assert(std::is_trivially_copyable_t<Paylopad>);

  Payload         mPayload;
  LogFormatEnd    mFormatEnd;
  TaskId          mTaskId;
  MessageSequence mMessageSequence;

public:
  MessageVariant() = default;
  MessageVariant(MessageVariant const &) = default;
  MessageVariant(MessageVariant &&) = default;
  MessageVariant& operator=(MessageVariant const &) = default;
  MessageVariant& operator=(MessageVariant &&) = default;

  template<typename tArgument>
  void set(tArgument const aValue, LogFormatEnd const aFormatEnd, TaskId const aTaskId, MessageSequence const aMessageSequence) noexcept {
    mPayload = aValue;
    mFormatEnd = aFormatEnd;
    mTaskId = aTaskId;
    mMessageSequence = aMessageSequence;
  }

  template<typename tDispatcher>
  void output() const noexcept {
    auto visitor = [](const auto aObj) { tDispatcher::transfer(aObj); };
    std::visit(visitor, mPayload);
  }

  bool isTerminal() const noexcept {
    return mFormatEnd.isTerminal();
  }

  uint8_t getBase() const noexcept {
    return mFormatEnd.getBase();
  }  

  uint8_t getFill() const noexcept {
    return mFormatEnd.getFill();
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