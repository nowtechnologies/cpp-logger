
#ifndef NOWTECH_LOG_MESSAGE_BASE
#define NOWTECH_LOG_MESSAGE_BASE

#include "LogMessageBase.h"
#include <array>

namespace nowtech::log {

// Will be copied via taking the data pointer. Here we go for size.
template<bool tSupport64>
class MessageCompact final : public MessageBase<tSupport64> {
private:
  enum class Type : uint8_t {
    cInvalid, cFloat, cDouble, cUint8_t, cUint16_t, cUint32_t, cUint64_t, cInt8_t, cInt16_t, cInt32_t, cInt64_t, cChar;
  };

public:
  static constexpr size_t csTotalSize = sizeof(LargestPayload) + 2 * sizeof(uint8_t) + sizeof(TaskId) + sizeof(MessageSequence) + sizeof(Type);

private:
  static constexpr size_t csOffsetPayload         = 0u;
  static constexpr size_t csOffsetBaseEnd         = csOffsetPayload + sizeof(LargestPayload);
  static constexpr size_t csOffsetFill            = csOffsetBaseEnd + sizeof(uint8_t);
  static constexpr size_t csOffsetTaskId          = csOffsetFill + sizeof(uint8_t);
  static constexpr size_t csOffsetMessageSequence = csOffsetTaskId + sizeof(TaskId);
  static constexpr size_t csOffsetType            = csOffsetMessageSequence + sizeof(MessageSequence);
  
  uint8_t mData[csTotalSize];

public:
  MessageCompact() = default;
  MessageCompact(MessageCompact const &) = default;
  MessageCompact(MessageCompact &&) = default;
  MessageCompact& operator=(MessageCompact const &) = default;
  MessageCompact& operator=(MessageCompact &&) = default;

  template<typename tArgument> Type getType() const noexcept { return Type::cInvalid; }
  template<> Type getType<float>() const noexcept { return Type::cFloat; }
  template<> Type getType<double>() const noexcept { return Type::cDouble; }
  template<> Type getType<uint8_t>() const noexcept { return Type::cUint8_t; }
  template<> Type getType<uint16_t>() const noexcept { return Type::cUint16_t; }
  template<> Type getType<uint32_t>() const noexcept { return Type::cUint32_t; }
  template<> Type getType<uint64_t>() const noexcept { return Type::cUint64_t; }
  template<> Type getType<int8_t>() const noexcept { return Type::cInt8_t; }
  template<> Type getType<int16_t>() const noexcept { return Type::cInt16_t; }
  template<> Type getType<int32_t>() const noexcept { return Type::cInt32_t; }
  template<> Type getType<int64_t>() const noexcept { return Type::cInt64_t; }
  template<> Type getType<char*>() const noexcept { return Type::cChar; }

  uint8_t data() noexcept {
    return mData;
  }

  void invalidate(MessageSequence const aMessageSequence) noexcept {
    mData[csOffsetType] = static_cast<uint8_t>(Type::cInvalid);
    mData[csOffsetMessageSequence] = aMessageSequence;
  }

  bool isValid() const noexcept {
    return mData[csOffsetType] != static_cast<uint8_t>(Type::cInvalid);
  }

  template<typename tArgument>
  void set(tArgument const aValue, LogFormatEnd const aFormatEnd, TaskId const aTaskId, MessageSequence const aMessageSequence) noexcept {
    std::memcpy(mData + csOffsetPayload, &aValue, sizeof(aValue));
    setRest(aFormatEnd, aTaskId, aMessageSequence);
    Type type = getType<tArgument>();
    mData[csOffsetType] = static_cast<uint8_t>(type);
  }

  template<typename tDispatcher>
  void output() const noexcept {
    Type type = static_cast<Type>(mData[csOffsetType]);
    if(type == Type::cFloat) {
      float value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      tDispatcher::transfer(aValue);
    }
    else if(type == Type::cUint8_t) {
      tDispatcher::transfer(mData[csOffsetPayload]);
    }
    else if(type == Type::cUint16_t) {
      uint16_t value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      tDispatcher::transfer(aValue);
    }
    else if(type == Type::cUint32_t) {
      uint32_t value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      tDispatcher::transfer(aValue);
    }
    else if(type == Type::cInt8_t) {
      int8_t value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      tDispatcher::transfer(aValue);
    }
    else if(type == Type::cInt16_t) {
      int16_t value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      tDispatcher::transfer(aValue);
    }
    else if(type == Type::cInt32_t) {
      int32_t value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      tDispatcher::transfer(aValue);
    }
    else if(type == Type::cChar) {
      char* value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      tDispatcher::transfer(aValue);
    }
    else { // nothing to do
    }
    if constexpr(tSupport64) {
      output64<tDispatcher>();
    }
    else { // nothing to do
    }
  }

  bool isTerminal() const noexcept {
    return LogFormatEnd::uint2terminal(mData[csOffsetBaseEnd]);
  }

  uint8_t getBase() const noexcept {
    return LogFormatEnd::uint2base(mData[csOffsetBaseEnd]);
  }  

  uint8_t getFill() const noexcept {
    return mData[csOffsetFill];
  }  

  TaskId getTaskId() const noexcept {
    return mData[csOffsetTaskId];
  }  

  MessageSequence getMessageSequence() const noexcept {
    return mData[csOffsetMessageSequence];
  }  

private:
  void setRest (LogFormatEnd const aFormatEnd, TaskId const aTaskId, MessageSequence const aMessageSequence) noexcept {
    mData[csOffsetBaseEnd] = aFormatEnd.getBaseEnd();
    mData[csOffsetFill] = aFormatEnd.getFill();
    mData[csOffsetTaskId] = aTaskId;
    mData[csOffsetMessageSequence] = aMessageSequence;
  }

  template<bool tSupport64inner = tSupport64>
  typename std::enable_if_t<tSupport64inner> 
  template<typename tDispatcher>
  void output64() {
    Type type = static_cast<Type>(mData[csOffsetType]);
    if(type == Type::cDouble) {
      double value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      tDispatcher::transfer(aValue);
    }
    else if(type == Type::cUint64_t) {
      uint64_t value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      tDispatcher::transfer(aValue);
    }
    else if(type == Type::cInt64_t) {
      int64_t value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      tDispatcher::transfer(aValue);
    }
    else { // nothing to do
    }
  }
};

}

#endif