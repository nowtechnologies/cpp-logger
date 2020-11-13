
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
    cInvalid, cFloat, cDouble, cUint8_t, cUint16_t, cUint32_t, cUint64_t, cInt8_t, cInt16_t, cInt32_t, cInt64_t, cChar, cCharArray;
  };

public:
  static constexpr size_t csTotalSize = sizeof(LargestPayload) + 2 * sizeof(uint8_t) + sizeof(TaskId) + sizeof(MessageSequence) + sizeof(Type);

private:
  static constexpr size_t csOffsetPayload         = 0u;
  static constexpr size_t csOffsetBase         = csOffsetPayload + sizeof(LargestPayload);
  static constexpr size_t csOffsetFill            = csOffsetBase + sizeof(uint8_t);
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
  template<> Type getType<char>() const noexcept { return Type::cChar; }
  template<> Type getType<char*>() const noexcept { return Type::cCharArray; }

  uint8_t data() noexcept {
    return mData;
  }

  template<typename tArgument>
  void set(tArgument const aValue, LogFormat const aFormat, TaskId const aTaskId, MessageSequence const aMessageSequence) noexcept {
    std::memcpy(mData + csOffsetPayload, &aValue, sizeof(aValue));
    setRest(aFormat, aTaskId, aMessageSequence);
    Type type = getType<tArgument>();
    mData[csOffsetType] = static_cast<uint8_t>(type);
  }

  template<typename tConverter>
  void output(tConverter& aConverter) const noexcept {
    Type type = static_cast<Type>(mData[csOffsetType]);
    uint8_t base = LogFormat::uint2base(mData[csOffsetBase]);
    uint8_t fill = mData[csOffsetFill];

    if(type == Type::cFloat) {
      float value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      aConverter.convert(value, base, fill);
    }
    else if(type == Type::cUint8_t) {
      aConverter.convert(mData[csOffsetPayload], base, fill);
    }
    else if(type == Type::cUint16_t) {
      uint16_t value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      aConverter.convert(value, base, fill);
    }
    else if(type == Type::cUint32_t) {
      uint32_t value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      aConverter.convert(value, base, fill);
    }
    else if(type == Type::cInt8_t) {
      int8_t value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      aConverter.convert(value, base, fill);
    }
    else if(type == Type::cInt16_t) {
      int16_t value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      aConverter.convert(value, base, fill);
    }
    else if(type == Type::cInt32_t) {
      int32_t value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      aConverter.convert(value, base, fill);
    }
    else if(type == Type::cChar) {
      char value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      aConverter.convert(value, base, fill);
    }
    else if(type == Type::cCharArray) {
      char* value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      aConverter.convert(value, base, fill);
    }
    else { // nothing to do
    }
    if constexpr(tSupport64) {
      output64<tConverter>(aConverter, base, fill);
    }
    else { // nothing to do
    }
  }

  bool isTerminal() const noexcept {
    return mData[csOffsetMessageSequence] == csTerminal;
  }

  uint8_t getBase() const noexcept {
    return LogFormat::uint2base(mData[csOffsetBase]);
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
  void setRest (LogFormat const aFormat, TaskId const aTaskId, MessageSequence const aMessageSequence) noexcept {
    mData[csOffsetBase] = aFormat.getBase();
    mData[csOffsetFill] = aFormat.getFill();
    mData[csOffsetTaskId] = aTaskId;
    mData[csOffsetMessageSequence] = aMessageSequence;
  }

  template <typename tConverter, typename tDummy = void>
  auto output64(tConverter& aConverter, uint8_t const aBase, uint8_t const aFill) -> std::enable_if_t<tSupport64, tDummy> const noexcept {
    Type type = static_cast<Type>(mData[csOffsetType]);
    if(type == Type::cDouble) {
      double value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      aConverter.convert(value, aBase, aFill);
    }
    else if(type == Type::cUint64_t) {
      uint64_t value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      aConverter.convert(value, aBase, aFill);
    }
    else if(type == Type::cInt64_t) {
      int64_t value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      aConverter.convert(value, aBase, aFill);
    }
    else { // nothing to do
    }
  }
};

}

#endif