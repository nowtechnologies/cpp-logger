
#ifndef NOWTECH_LOG_MESSAGE_COMPACT
#define NOWTECH_LOG_MESSAGE_COMPACT

#include "LogMessageBase.h"
#include <array>
#include <cstring>

namespace nowtech::log {

// Will be copied via taking the data pointer. Here we go for size.
template<size_t tPayloadSize>
class MessageCompact final : public MessageBase<tPayloadSize> {
public:
  static constexpr size_t csPayloadSize = tPayloadSize + sizeof(uint8_t); // Antipattern to use the base field for storage, but we go for space saving.

private:
  enum class Type : uint8_t {
    cInvalid, cBool, cFloat, cDouble, cLongDouble, cUint8_t, cUint16_t, cUint32_t, cUint64_t, cInt8_t, cInt16_t, cInt32_t, cInt64_t, cChar, cCharArray, cStoredChars
  };

  static constexpr size_t csTotalSize             = tPayloadSize + 2 * sizeof(uint8_t) + sizeof(TaskId) + sizeof(MessageSequence) + sizeof(Type);
  static constexpr size_t csOffsetPayload         = 0u;
  static constexpr size_t csOffsetBase            = csOffsetPayload + tPayloadSize;
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
  template<> Type getType<bool>() const noexcept { return Type::cBool; }
  template<> Type getType<float>() const noexcept { return Type::cFloat; }
  template<> Type getType<double>() const noexcept { return Type::cDouble; }
  template<> Type getType<long double>() const noexcept { return Type::cLongDouble; }
  template<> Type getType<uint8_t>() const noexcept { return Type::cUint8_t; }
  template<> Type getType<uint16_t>() const noexcept { return Type::cUint16_t; }
  template<> Type getType<uint32_t>() const noexcept { return Type::cUint32_t; }
  template<> Type getType<uint64_t>() const noexcept { return Type::cUint64_t; }
  template<> Type getType<int8_t>() const noexcept { return Type::cInt8_t; }
  template<> Type getType<int16_t>() const noexcept { return Type::cInt16_t; }
  template<> Type getType<int32_t>() const noexcept { return Type::cInt32_t; }
  template<> Type getType<int64_t>() const noexcept { return Type::cInt64_t; }
  template<> Type getType<char>() const noexcept { return Type::cChar; }
  template<> Type getType<char *>() const noexcept { return Type::cCharArray; }
  template<> Type getType<char * const>() const noexcept { return Type::cCharArray; }
  template<> Type getType<char const *>() const noexcept { return Type::cCharArray; }
  template<> Type getType<char const * const>() const noexcept { return Type::cCharArray; }
  template<> Type getType<std::array<char, csPayloadSize>>() const noexcept { return Type::cStoredChars; }
  template<> Type getType<std::array<char, csPayloadSize> const>() const noexcept { return Type::cStoredChars; }

  uint8_t data() noexcept {
    return mData;
  }

  template<typename tArgument>
  void set(tArgument const aValue, LogFormat const aFormat, TaskId const aTaskId, MessageSequence const aMessageSequence) noexcept {
    std::memcpy(mData + csOffsetPayload, &aValue, sizeof(aValue));
    Type type = getType<tArgument>();
    mData[csOffsetType] = static_cast<uint8_t>(type);
    mData[csOffsetFill] = aFormat.mFill;
    mData[csOffsetTaskId] = aTaskId;
    mData[csOffsetMessageSequence] = aMessageSequence;
    if(type != Type::cStoredChars) {
      mData[csOffsetBase] = aFormat.mBase;
    }
    else { // nothing to do
    }
  }

  template<typename tConverter>
  void output(tConverter& aConverter) const noexcept {
    Type type = static_cast<Type>(mData[csOffsetType]);
    uint8_t base = mData[csOffsetBase];
    uint8_t fill = mData[csOffsetFill];

    if(type == Type::cFloat) {
      float value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      aConverter.convert(value, base, fill);
    }
    else if(type == Type::cBool) {
      bool value;
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
    else if(type == Type::cStoredChars) {
      aConverter.convert(reinterpret_cast<char const*>(mData + csOffsetPayload), base, fill);
    }
    else {
      if constexpr(tPayloadSize >= sizeof(int64_t) || sizeof(char*) > sizeof(int32_t)) {
        output64<tConverter>(aConverter, base, fill);
      }
      else { // nothing to do
      }
      if constexpr(tPayloadSize > sizeof(long double)) {
        output80<tConverter>(aConverter, base, fill);
      }
      else { // nothing to do
      }
    }
  }

  bool isTerminal() const noexcept {
    return mData[csOffsetMessageSequence] == MessageBase<tPayloadSize>::csTerminal;
  }

  uint8_t getBase() const noexcept {
    return mData[csOffsetBase];
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
  template<typename tConverter>
  void output64(tConverter& aConverter, uint8_t const aBase, uint8_t const aFill) const noexcept {
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

  template<typename tConverter>
  void output80(tConverter& aConverter, uint8_t const aBase, uint8_t const aFill) const noexcept {
    Type type = static_cast<Type>(mData[csOffsetType]);
    if(type == Type::cLongDouble) {
      long double value;
      std::memcpy(&value, mData + csOffsetPayload, sizeof(value));
      aConverter.convert(value, aBase, aFill);
    }
    else { // nothing to do
    }
  }
};

}

#endif