//
// Copyright 2018-2020 Now Technologies Zrt.
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software,
// and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
// THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#ifndef NOWTECH_LOG_INCLUDED
#define NOWTECH_LOG_INCLUDED

#include <cstdint>
#include <type_traits>
#include <atomic>
#include <limits>
#include <cmath>
#include <map>

namespace nowtech::log {

namespace NumericSystem {
  static constexpr uint8_t cBinary      =  2u;
  static constexpr uint8_t cDecimal     = 10u;
  static constexpr uint8_t cHexadecimal = 16u;
}

enum class Exception : uint8_t {
  cOutOfTaskIds = 0u,
  cCount        = 1u
};

typedef uint8_t TaskIdType;
typedef int8_t LogTopicType;

template<typename tAppInterface, typename tInterface, TaskIdType tMaxTaskCount, uint8_t tSizeofIntegerTransform, uint8_t tAppendStackBufferLength>
class Log;

class LogTopicInstance final {
  template<typename tAppInterface, typename tInterface, TaskIdType tMaxTaskCount, uint8_t tSizeofIntegerTransform, uint8_t tAppendStackBufferLength>
  friend class Log;

public:
  static constexpr LogTopicType cInvalidTopic = 0u;

private:
  LogTopicType mValue = cInvalidTopic;

  LogTopicType operator=(LogTopicType const aValue) {
    mValue = aValue;
    return aValue;
  }

public:
  LogTopicType operator*() {
    return mValue;
  }

  operator LogTopicType() {
    return mValue;
  }
};

/// Struct holding numeric system and zero aFill information.
struct LogFormat {
  /// Base of the numeric system. Can be 2, 10, 16.
  uint8_t aBase;

  /// Number of digits to emit with zero aFill, or 0 if no aFill.
  uint8_t aFill;

  /// Constructor.
  constexpr LogFormat()
  : aBase(0u)
  , aFill(0u) {
  }

  /// Constructor.
  constexpr LogFormat(uint8_t const aBase, uint8_t const aFill)
  : aBase(aBase)
  , aFill(aFill) {
  }

  LogFormat(LogFormat const &) = default;
  LogFormat(LogFormat &&) = default;
  LogFormat& operator=(LogFormat const &) = default;
  LogFormat& operator=(LogFormat &&) = default;

  bool isValid() const noexcept {
    return (aBase == NumericSystem::cBinary || aBase == NumericSystem::cDecimal) || aBase == NumericSystem::cHexadecimal;
  }
};

/// Configuration struct with default values for general usage.
struct LogConfig final {
public:
  /// Type of info to log about the sender task
  enum class TaskRepresentation : uint8_t {cNone, cId, cName};

  /// This is the default logging format and the only one I will document
  /// here. For the others, the letter represents the aBase of the number
  /// system and the number represents the minimum digits to write, possibly
  /// with leading zeros. When formats are applied to floating point
  /// numbers, the numeric system info is discarded.
  inline static constexpr LogFormat cDefault = LogFormat(10, 0);
  inline static constexpr LogFormat cNone = LogFormat(0, 0);
  inline static constexpr LogFormat cB4 = LogFormat(2, 4);
  inline static constexpr LogFormat cB8 = LogFormat(2, 8);
  inline static constexpr LogFormat cB12 = LogFormat(2, 12);
  inline static constexpr LogFormat cB16 = LogFormat(2, 16);
  inline static constexpr LogFormat cB24 = LogFormat(2, 24);
  inline static constexpr LogFormat cB32 = LogFormat(2, 32);
  inline static constexpr LogFormat cD1 = LogFormat(10, 1);
  inline static constexpr LogFormat cD2 = LogFormat(10, 2);
  inline static constexpr LogFormat cD3 = LogFormat(10, 3);
  inline static constexpr LogFormat cD4 = LogFormat(10, 4);
  inline static constexpr LogFormat cD5 = LogFormat(10, 5);
  inline static constexpr LogFormat cD6 = LogFormat(10, 6);
  inline static constexpr LogFormat cD7 = LogFormat(10, 7);
  inline static constexpr LogFormat cD8 = LogFormat(10, 8);
  inline static constexpr LogFormat cX1 = LogFormat(16, 1);
  inline static constexpr LogFormat cX2 = LogFormat(16, 2);
  inline static constexpr LogFormat cX3 = LogFormat(16, 3);
  inline static constexpr LogFormat cX4 = LogFormat(16, 4);
  inline static constexpr LogFormat cX6 = LogFormat(16, 6);
  inline static constexpr LogFormat cX8 = LogFormat(16, 8);

  /// If true, task registration will be sent to the output in the form
  /// in the form -=- Registered task: taskname (1) -=-
  bool allowRegistrationLog = true;

  /// If true, logging will work from ISR.
  bool logFromIsr = false;

  /// Length of a FreeRTOS queue in chunks.
  uint32_t queueLength = 64u;

  /// Length of the circular buffer used for message sorting, measured also in
  /// chunks.
  uint32_t circularBufferLength = 64u;

  /// Length of a buffer in the transmission double-buffer pair, in chunks.
  uint32_t transmitBufferLength = 32u;

  /// Length of a pause in ms during waiting for transmission of the other
  /// buffer or timeout while reading from the FreeRTOS queue.
  uint32_t pauseLength = 100u;

  /// Length of the period used to wait for messages before transmitting a partially
  /// filled transmission buffer. The shorter the aValue the more prompt the display.
  uint32_t refreshPeriod = 1000u;

  /// Signs if writing the FreeRTOS queue can block or should return on the expense
  /// of losing chunks. Note, that even in blocking mode the throughput can not
  /// reach the theoretical UART bps limit.
  /// Important! In non-blocking mode high demands will result in loss of complete
  /// messages and often an internal lockup of the log system.
  bool blocks = true;

  /// Representation of a task in the message header, if any. It can be missing,
  /// numeric task ID or FreeRTOS task name.
  TaskRepresentation taskRepresentation = TaskRepresentation::cId;

  /// True if number formatter should append 0b or 0x
  bool appendBasePrefix = false;

  /// Format for displaying the task ID in the message header.
  LogFormat taskIdFormat = cX2;

  /// Format for displaying the FreeRTOS ticks in the header, if any. Should be
  /// LogFormat::cNone to disable tick output.
  LogFormat tickFormat   = cD5;

  /// These are default formats for some types.
  LogFormat int8Format   = cDefault;
  LogFormat int16Format  = cDefault;
  LogFormat int32Format  = cDefault;
  LogFormat int64Format  = cDefault;
  LogFormat uint8Format  = cDefault;
  LogFormat uint16Format = cDefault;
  LogFormat uint32Format = cDefault;
  LogFormat uint64Format = cDefault;
  LogFormat floatFormat  = cD5;
  LogFormat doubleFormat = cD8;

  /// If true, positive numbers will be prepended with a space to let them align negatives.
  bool alignSigned = false;

  LogConfig() noexcept = default;
};

/// Auxiliary class, not part of the Log API.
class Chunk final {
private:
  LogOsInterface *tInterface;
  char * mOrigin;
  char * mChunk;
  LogSizeType mChunkSize;
  LogSizeType mBufferBytes;
  LogSizeType mIndex = 1;
  bool mBlocks;

public:
  Chunk(LogOsInterface * const aOsInterface, char * const aChunk, LogSizeType const aBufferLength) noexcept
    : tInterface(aOsInterface)
    , mOrigin(aChunk)
    , mChunk(aChunk)
    , mChunkSize(aOsInterface->getChunkSize())
    , mBufferBytes(aBufferLength * mChunkSize)
    , mBlocks(true) {
  }

  Chunk(LogOsInterface * const aOsInterface
    , char * const aChunk
    , LogSizeType const aBufferLength
    , TaskIdType const aTaskId) noexcept
    : tInterface(aOsInterface)
    , mOrigin(aChunk)
    , mChunk(aChunk)
    , mChunkSize(aOsInterface->getChunkSize())
    , mBufferBytes(aBufferLength * mChunkSize)
    , mBlocks(true) {
    mChunk[0] = *reinterpret_cast<char const *>(&aTaskId);
  }

  char * getData() const noexcept {
    return mChunk;
  }

  TaskIdType getTaskId() const noexcept {
    return *reinterpret_cast<TaskIdType*>(mChunk);
  }

  char * const operator++() noexcept {
    mIndex = 1u;
    mChunk += mChunkSize;
    if(mChunk == (mOrigin + mBufferBytes)) {
      mChunk = mOrigin;
    }
    else { // nothing to do
    }
    return mChunk;
  }

  Chunk& operator=(char * const aStart) noexcept {
    mIndex = 1u;
    mChunk = const_cast<char*>(aStart);
    return *this;
  }

  Chunk& operator=(Chunk const& aChunk) noexcept {
    mIndex = 1u;
    for (LogSizeType i = 0u; i < mChunkSize; i++) {
      mChunk[i] = aChunk.mChunk[i];
    }
    return *this;
  }

  void invalidate() noexcept {
    mIndex = 1u;
    mChunk[0] = static_cast<char>(cInvalidTaskId);
  }

  void pop() noexcept {
    mIndex = 1u;
    if(!tInterface->pop(mChunk)) {
      mChunk[0] = static_cast<char>(cInvalidTaskId);
    }
    else { // nothing to do
    }
  }
};

/// Dummy type to use in << chain as end marker.
enum class LogShiftChainMarker : uint8_t {
  cEnd      = 0u
};


// template<typename tLogSizeType, bool tBlocks = false, tLogSizeType tChunkSize = 8u>
// class LogInterface {
// public:
//   typedef tLogSizeType LogSizeType;
//   static constexpr tLogSizeType cChunkSize = tChunkSize;
// };

template<typename tAppInterface, typename tInterface, TaskIdType tMaxTaskCount, uint8_t tSizeofIntegerConversion = 4u, uint8_t tAppendStackBufferLength = 70u>
class Log final {
  static_assert(tMaxTaskCount < std::numeric_limits<TaskIdType>::max());
  static_assert(tSizeofIntegerConversion == 2u || tSizeofIntegerConversion == 4u || tSizeofIntegerConversion == 8u);

private:
  typedef typename std::conditional_t<tSizeofIntegerTransform == 8u, uint64_t, uint32_t>::type tIntegerConversionUnsigned;
  typedef typename std::conditional_t<tSizeofIntegerTransform == 8u, int64_t, int32_t>::type tIntegerConversionSigned;
  typedef typename tInterface::LogSizeType LogSizeType;
  static_assert(std::is_unsigned<LogSizeType>);

  static constexpr TaskIdType cInvalidTaskId  = std::numeric_limits<TaskIdType>::max();
  static constexpr TaskIdType cLocalTaskId    = tMaxTaskCount;
  static constexpr TaskIdType cMaxTaskIdCount = tMaxTaskCount + 1u;

  class Appender final {
  private:
    LogSizeType mIndex = 1u;
    char mChunk[tInterface::cChunkSize];

  public:
    void Appender(TaskIdType const aTaskId) noexcept : mIndex(1u) {
      mChunk[0u] = aTaskId;
    }

    void startWithTaskId(TaskIdType const aTaskId) noexcept {
      mIndex = 1u;
      mChunk[0u] = aTaskId;
    }

    bool isValid() const noexcept {
      return mChunk[0u] != cInvalidTaskId;
    }

    void invalidate() noexcept {
      mChunk[0u] = cInvalidTaskId;
    }

    void push(char const aChar) noexcept {
      mChunk[mIndex] = aChar;
      ++mIndex;
      if(mIndex == tInterface::cChunkSize) {
        tInterface::push(mChunk);
        mIndex = 1u;
      }
      else { // nothing to do
      }
    }

    void flush() noexcept {
      mChunk[mIndex] = cEndOfMessage;
      tInterface::push(mChunk);
      mIndex = 1u;
    }
  };

  /// Dummy type to use in << chain as end marker.
  enum class LogShiftChainMarker : uint8_t {
    cEnd      = 0u
  };

  class LogShiftChainHelper final {
    Appender& mAppender;
    LogFormat mNextFormat;

  public:
    LogShiftChainHelper() noexcept = delete;

    LogShiftChainHelper(Appender& aAppender) noexcept : mAppender(aAppender) {
    }

    template<typename ArgumentType>
    LogShiftChainHelper& operator<<(ArgumentType const aValue) noexcept {
      if(mAppender.isValid() && mNextFormat.isValid()) {
        append(mAppender, mNextFormat, aValue);
        mNextFormat.aBase = 0u;
      }
      else { // nothing to do
      }
      return *this;
    }

    LogShiftChainHelper& operator<<(LogFormat const &aFormat) noexcept {
      mNextFormat = aFormat;
      return *this;
    }

    void operator<<(LogShiftChainMarker const) noexcept {
      if(mAppender.isValid()) {
        mAppender.flush();
      }
      else { // nothing to do
      }
    }
  };
  friend class LogShiftChainHelper;

  static constexpr uint32_t cNameLength  =  8u;

public:
  /// Will be used as Log << something << to << log << Log::end;
  static constexpr LogShiftChainMarker end = LogShiftChainMarker::cEnd;
/*
  /// Output for unknown LogTopicType parameter
  inline static constexpr char cUnknownApplicationName[cNameLength] = "UNKNOWN";
*/
private:
  static constexpr LogTopicType cFreeTopicIncrement = 1u;
  static constexpr LogTopicType cFirstFreeTopic = LogTopicInstance::cInvalidTopic + cFreeTopicIncrement;

  /// The character to use to sign a failure during the number to text
  /// conversion. There is no distinction between illegal aBase (neither of 2,
  /// 10, 16) or overflow of the conversion buffer.
  static constexpr char cNumericError            = '#';

  static constexpr char cIsrTaskName             = '?';
  static constexpr char cEndOfMessage            = '\r';
  static constexpr char cEndOfLine               = '\n';
  static constexpr char cNumericFill             = '0';
  static constexpr char cNumericMarkBinary       = 'b';
  static constexpr char cNumericMarkHexadecimal  = 'x';
  static constexpr char cMinus                   = '-';
  static constexpr char cSpace                   = ' ';
  static constexpr char cSeparatorFailure        = '@';
  static constexpr char cFractionDot             = '.';
  static constexpr char cPlus                    = '+';
  static constexpr char cScientificE             = 'e';


  inline static constexpr char cNan[]            = "nan";
  inline static constexpr char cInf[]            = "inf";
  inline static constexpr char cRegisteredTask[] = "-=- Registered task: ";

  /// Used to convert digits to characters.
  inline static constexpr char cDigit2char[NumericSystem::cHexadecimal] = {
    '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
  };

  /// Can be used to shut off the transmitter thread, if any
  inline static std::atomic<bool> sKeepRunning = true;

  /// The user-defined configuration values for message header and number
  /// rendering and else.
  inline static LogConfig const * sConfig;
  inline static TaskIdType sNextTaskId = 1u;

  inline static std::atomic<LogTopicType> sNextFreeTopic = cFirstFreeTopic;

  /// Map used to turn OS-specific task IDs into the artificial counterparts.
  inline static std::map<uint32_t, TaskIdType> sTaskIds;

  /// Registry to check calls like Log::send(nowtech::LogTopicType::cSystem, "stuff to log")
  inline static std::map<LogTopicType, char const *> sRegisteredTopics;

  inline static Appender* sShiftChainingAppenders;

public:
  static void init(LogConfig const &aConfig) {
    sConfig = *aConfig;
    tInterface::init(aConfig, [](){ transmitterThreadFunction(); });
    sShiftChainingAppenders = tAppInterface::_newArray<Appender>(cMaxTaskIdCount);
  }

  /// Does nothing, because this object is not intended to be destroyed.
  static void done() {
    sKeepRunning = false;
    tInterface::done();
    tAppInterface::deleteArray<Appender>(sShiftChainingCallBuffers);
  }

  /// Registers the current task if not already present. It can register
  /// at most 255 tasks. All others will be handled as one.
  /// NOTE: this method locks to inhibit concurrent access of methods with the same name.
  static void registerCurrentTask() noexcept {
    registerCurrentTask(nullptr);
  }

  /// Registers the current task if not already present. It can register
  /// at most 255 tasks. All others will be handled as one.
  /// NOTE: this method locks to inhibit concurrent access of methods with the same name.
  /// @param aTaskName Task name to use, when the osInterface supports it.
  static void registerCurrentTask(char const * const aTaskName) noexcept {
    tInterface::lock();
    if(sNextTaskId != cLocalTaskId) {
      if(aTaskName != nullptr) {
        tInterface::registerThreadName(aTaskName);
      }
      else { // nothing to do
      }
      uint32_t taskHandle = tInterface::getCurrentThreadId();
      auto found = sTaskIds.find(taskHandle);
      if(found == sTaskIds.end()) {
        sTaskIds[taskHandle] = sNextTaskId;
        if(sConfig->allowRegistrationLog) {
          Appender appender;
          append(appender, cRegisteredTasks);
          append(appender, tInterface::getThreadName(taskHandle));
          append(appender, cSpace);
          append(appender, sNextTaskId);
          appender.flush();
        }
        else { // nothing to do
        }
        ++sNextTaskId;
      }
      else { // nothing to do
      }
    }
    else {
      tAppInterface::fatalError(Exception::cOutOfTaskIds);
    }
    tInterface::unlock();
  }

  /// Registers the current log application
  /// at most 255 tasks. All others will be handled as one.
  static void registerTopic(LogTopicInstance &aTopic, char const * const aPrefix) noexcept {
    aTopic = sNextFreeTopic.fetch_add(cFreeTopicIncrement);
    sRegisteredTopics[aTopic] = aPrefix;
  }

  /// Returns true if the given app was registered.
  static bool isRegistered(LogTopicType const aTopic) noexcept {
    return sRegisteredTopics.find(aTopic) != sRegisteredTopics.end();
  }

// TODO supplement .cpp for FreeRTOS with extern "C"
  /// Transmitter thread implementation.
  void transmitterThreadFunction() noexcept {
    // we assume all the buffers are valid
    CircularBuffer circularBuffer(tInterface, sConfig->circularBufferLength, mChunkSize);
    TransmitBuffers transmitBuffers(tInterface, sConfig->transmitBufferLength, mChunkSize);
    while(mKeepRunning.load()) {
      // At this point the transmitBuffers must have free space for a chunk
      if(!transmitBuffers.hasActiveTask()) {
        if(circularBuffer.isEmpty()) {
          static_cast<void>(transmitBuffers << circularBuffer.fetch());
        }
        else { // the circularbuffer may be full or not
          static_cast<void>(transmitBuffers << circularBuffer.peek());
          circularBuffer.pop();
        }
      }
      else { // There is a task in the transmitBuffers to be continued
        if(circularBuffer.isEmpty() || !circularBuffer.isFull() && circularBuffer.isInspected()) {
          Chunk const &chunk = circularBuffer.fetch();
          if(chunk.getTaskId() != nowtech::cInvalidTaskId) {
            if(transmitBuffers.getActiveTaskId() == chunk.getTaskId()) {
              transmitBuffers << chunk;
            }
            else {
              circularBuffer.keepFetched();
            }
          }
          else { // nothing to do
          }
        }
        else if(!circularBuffer.isFull() && !circularBuffer.isInspected()) {
          Chunk const &chunk = circularBuffer.inspect(transmitBuffers.getActiveTaskId());
          if(!circularBuffer.isInspected()) {
            transmitBuffers << chunk;
            circularBuffer.removeFound();
          }
          else { // nothing to do
          }
        }
        else { // the circular buffer is full
          static_cast<void>(transmitBuffers << circularBuffer.peek());
          circularBuffer.pop();
          circularBuffer.clearInspected();
        }
      }
      if(transmitBuffers.gotTerminalChunk()) {
        circularBuffer.clearInspected();
      }
      else {
      }
      transmitBuffers.transmitIfNeeded();
    }
  }

  static LogShiftChainHelper i(TaskIdType const aTaskId = cLocalTaskId) noexcept {
    TaskIdType const taskId = getCurrentTaskId(aTaskId);
    Appender& appender = sShiftChainingAppenders[taskId];
    appender.startWithTaskId(taskId);
    startSend(appender);
    return nowtech::LogShiftChainHelper(appender);
  }

  static LogShiftChainHelper i(LogTopicType const aTopic, TaskIdType const aTaskId = cLocalTaskId) noexcept {
    TaskIdType const taskId = getCurrentTaskId(aTaskId);
    Appender& appender = sShiftChainingAppenders[taskId];
    appender.startWithTaskId(taskId);
    startSend(appender, aTopic);
    return nowtech::LogShiftChainHelper(appender);
  }

  static LogShiftChainHelper n(TaskIdType const aTaskId = cLocalTaskId) noexcept {
    TaskIdType const taskId = getCurrentTaskId(aTaskId);
    Appender& appender = sShiftChainingAppenders[taskId];
    appender.startWithTaskId(taskId);
    startSendNoHeader(appender);
    return nowtech::LogShiftChainHelper(appender);
  }

  static LogShiftChainHelper n(LogTopicType const aTopic, TaskIdType const aTaskId = cLocalTaskId) noexcept {
    TaskIdType const taskId = getCurrentTaskId(aTaskId);
    Appender& appender = sShiftChainingAppenders[taskId];
    appender.startWithTaskId(taskId);
    startSendNoHeader(appender, aTopic);
    return nowtech::LogShiftChainHelper(appender);
  }

  template<typename tValueType>
  static void send(LogTopicType const aTopic, LogFormat const &aFormat, tValueType const aValue, TaskIdType const aTaskId = cLocalTaskId) noexcept {
    Appender appender(aTaskId);
    startSend(appender, aTopic);
    if(appender.isValid()) {
      append(appender, aFormat, aValue);
      appender.flush();
    }
    else { // nothing to do
    }
  }

  template<typename tValueType>
  static void send(LogFormat const &aFormat, tValueType const aValue, TaskIdType const aTaskId = cLocalTaskId) noexcept {
    Appender appender(aTaskId);
    startSend(appender);
    append(appender, aFormat, aValue);
    appender.flush();
  }

  template<typename tValueType>
  static void send(LogTopicType const aTopic, tValueType const aValue, TaskIdType const aTaskId = cLocalTaskId) noexcept {
    Appender appender(aTaskId);
    startSend(appender, aTopic);
    if(appender.isValid()) {
      append(appender, aValue);
      appender.flush();
    }
    else { // nothing to do
    }
  }

  template<typename tValueType>
  static void send(tValueType const aValue, TaskIdType const aTaskId = cLocalTaskId) noexcept {
    Appender appender(aTaskId);
    startSend(appender);
    append(appender, aValue);
    appender.flush();
  }

  template<typename tValueType>
  static void sendNoHeader(LogTopicType const aTopic, LogFormat const &aFormat, tValueType const aValue, TaskIdType const aTaskId = cLocalTaskId) noexcept {
    Appender appender(aTaskId);
    startSendNoHeader(appender, aTopic);
    if(appender.isValid()) {
      append(appender, aFormat, aValue);
      appender.flush();
    }
    else { // nothing to do
    }
  }

  template<typename tValueType>
  static void sendNoHeader(LogFormat const &aFormat, tValueType const aValue, TaskIdType const aTaskId = cLocalTaskId) noexcept {
    Appender appender(aTaskId);
    startSendNoHeader(appender);
    append(appender, aFormat, aValue);
    appender.flush();
  }

  template<typename tValueType>
  static void sendNoHeader(LogTopicType const aTopic, tValueType const aValue, TaskIdType const aTaskId = cLocalTaskId) noexcept {
    Appender appender(aTaskId);
    startSendNoHeader(appender, aTopic);
    if(appender.isValid()) {
      append(appender, aValue);
      appender.flush();
    }
    else { // nothing to do
    }
  }

  template<typename tValueType>
  static void sendNoHeader(tValueType const aValue, TaskIdType const aTaskId = cLocalTaskId) noexcept {
    Appender appender(aTaskId);
    startSendNoHeader(appender);
    append(appender, aValue);
    appender.flush();
  }

private:
  TaskIdType getCurrentTaskId(TaskIdType const aTaskId) const noexcept {
    TaskIdType result = aTaskId;
    if(aTaskId == cLocalTaskId && !tInterface::isInterrupt()) {
      auto found = sTaskIds.find(tInterface::getCurrentThreadId());
      return found == sTaskIds.end() ? cInvalidTaskId : found->second;
    }
    else { // nothing to do
    }
    return result;
  }

  void startSend(Appender& aAppender) noexcept {
    startSendNoHeader(aAppender);
    if(appender.isValid()) {
      if(sConfig->taskRepresentation == LogConfig::TaskRepresentation::cId) {
        append(appender, *reinterpret_cast<uint8_t*>(appender.getData()), sConfig->taskIdFormat.aBase, sConfig->taskIdFormat.aFill);
        append(appender, cSpace);
      }
      else if(sConfig->taskRepresentation == LogConfig::TaskRepresentation::cName) {
        if(tInterface::isInterrupt()) {
          append(appender, cIsrTaskName);
        }
        else {
          append(appender, tInterface::getCurrentThreadName());
        }
        append(appender, cSpace);
      }
      else { // nothing to do
      }
      if(sConfig->tickFormat.aBase != 0) {
        append(appender, tInterface::getLogTime(), static_cast<uint32_t>(sConfig->tickFormat.aBase), sConfig->tickFormat.aFill);
        append(appender, cSpace);
      }
      else { // nothing to do
      }
    }
    else { // nothing to do
    }
  }

  void startSend(Appender& aAppender, LogTopicType aTopic) noexcept {
    auto found = mRegisteredTopics.find(aTopic);
    if(found != mRegisteredTopics.end()) {
      startSend(aAppender);
      append(appender, found->second);
      append(appender, cSpace);
    }
    else {
      aAppender.invalidate();
    }
  }

  void startSendNoHeader(Appender& aAppender) noexcept {
    if(tInterface::isInterrupt() && !sConfig->logFromIsr) {
      appender.invalidate();
    }
    else { // nothing to do
    }
  }

  void startSendNoHeader(Appender& aAppender, LogTopicType aTopic) noexcept {
    auto found = mRegisteredTopics.find(aTopic);
    if(found != mRegisteredTopics.end()) {
      startSendNoHeader(aAppender);
      append(appender, found->second);
      append(appender, cSpace);
    }
    else {
      aAppender.invalidate();
    }
  }

  void append(Appender &aAppender, LogFormat const & aFormat, int8_t const aValue) noexcept {
    append(aAppender, static_cast<IntegerConversionSigned>(aValue), static_cast<IntegerConversionSigned>(aFormat.aBase), aFormat.aFill);
  }

  void append(Appender &aAppender, LogFormat const & aFormat, int16_t const aValue) noexcept {
    append(aAppender, static_cast<IntegerConversionSigned>(aValue), static_cast<IntegerConversionSigned>(aFormat.aBase), aFormat.aFill);
  }

  void append(Appender &aAppender, LogFormat const & aFormat, int32_t const aValue) noexcept {
    append(aAppender, static_cast<IntegerConversionSigned>(aValue), static_cast<IntegerConversionSigned>(aFormat.aBase), aFormat.aFill);
  }

  void append(Appender &aAppender, LogFormat const & aFormat, int64_t const aValue) noexcept {
    append(aAppender, aValue, static_cast<int64_t>(aFormat.aBase), aFormat.aFill);
  }

  void append(Appender &aAppender, LogFormat const & aFormat, uint8_t const aValue) noexcept {
    append(aAppender, static_cast<IntegerConversionUnigned>(aValue), static_cast<IntegerConversionUnigned>(aFormat.aBase), aFormat.aFill);
  }

  void append(Appender &aAppender, LogFormat const & aFormat, uint16_t const aValue) noexcept {
    append(aAppender, static_cast<IntegerConversionUnigned>(aValue), static_cast<IntegerConversionUnigned>(aFormat.aBase), aFormat.aFill);
  }

  void append(Appender &aAppender, LogFormat const & aFormat, uint32_t const aValue) noexcept {
    append(aAppender, static_cast<IntegerConversionUnigned>(aValue), static_cast<IntegerConversionUnigned>(aFormat.aBase), aFormat.aFill);
  }

  void append(Appender &aAppender, LogFormat const & aFormat, uint64_t const aValue) noexcept {
    append(aAppender, aValue, static_cast<uint64_t>(aFormat.aBase), aFormat.aFill);
  }

  void append(Appender &aAppender, LogFormat const & aFormat, float const aValue) noexcept {
    append(aAppender, static_cast<double>(aValue), aFormat.aFill);
  }

  void append(Appender &aAppender, LogFormat const & aFormat, double const aValue) noexcept {
    append(aAppender, aValue, aFormat.aFill);
  }

/* TODO check if we cen go with compile errors template<typename tValueType>
  void append(Appender &aAppender, LogFormat const & aFormat, tValueType const aValue) noexcept {
    append(aAppender, "-=unknown=-");
  }*/

  void append(Appender &aAppender, bool const aBool) noexcept {
    if(aBool) {
      append(aAppender, "true");
    }
    else {
      append(aAppender, "false");
    }
  }

  /// Appends the character to the message. If this is the last to fit the
  /// buffer, appends a newline instead of the given character, and changes
  /// the preceding one to @ to show that the message is truncated and
  /// possibly other messages will be dropped.
  /// @param ch character to append.
  /// @return true if succeeded, false if truncation occurs or buffer was full.
  void append(Appender &aAppender, char const aCh) noexcept {
    aAppender.push(aCh);
  }

  /// Uses append(char const ch) to send the string character by character.
  /// @param string to send
  /// @return the return aValue of the last append(char const ch) call.
  void append(Appender &aAppender, char const * const aString) noexcept {
    char const * pointer = aString;
    if(pointer != nullptr) {
      while(*pointer != 0) {
        aAppender.push(*pointer);
        ++pointer;
      }
    }
    else { // nothing to do
    }
  }

  void append(Appender &aAppender, int8_t const aValue) noexcept {
    append(aAppender, sConfig->int8Format, aValue);
  }

  void append(Appender &aAppender, int16_t const aValue) noexcept {
    append(aAppender, Config->int16Format, aValue);
  }

  void append(Appender &aAppender, int32_t const aValue) noexcept {
    append(aAppender, Config->int32Format, aValue);
  }

  void append(Appender &aAppender, int64_t const aValue) noexcept {
    append(aAppender, Config->int64Format, aValue);
  }

  void append(Appender &aAppender, uint8_t const aValue) noexcept {
    append(aAppender, sConfig->uint8Format, aValue);
  }

  void append(Appender &aAppender, uint16_t const aValue) noexcept {
    append(aAppender, Config->uint16Format, aValue);
  }

  void append(Appender &aAppender, uint32_t const aValue) noexcept {
    append(aAppender, Config->uint32Format, aValue);
  }

  void append(Appender &aAppender, uint64_t const aValue) noexcept {
    append(aAppender, Config->uint64Format, aValue);
  }

  void append(Appender &aAppender, float const aValue) noexcept {
    append(aAppender, static_cast<double>(aValue), sConfig->floatFormat.aFill);
  }

  void append(Appender &aAppender, double const aValue) noexcept {
    append(aAppender, aValue, sConfig->doubleFormat.aFill);
  }

  /// Converts the number to string in a stack buffer and uses append(char
  /// const ch) to send it character by character. If the conversion fails
  /// (due to invalid base or too small buffer on stack) a # will be appended
  /// instead. For non-decimal numbers 0b or 0x is prepended.
  /// tValueType should be int32_t, uint32_t, int64_t or uint64_t to avoid too many template instantiations.
  /// @param aValue the number to convert
  /// @param aBase of the number system to use
  /// @param aFill number of digits to use at least. Shorter numbers will be
  /// filled using cNumericFill.
  /// @return the return aValue of the last append(char const ch) call.
  template<typename tValueType>
  void append(Appender &aAppender, tValueType const aValue, tValueType const aBase, uint8_t const aFill) noexcept {
    tValueType tmpValue = aValue;
    uint8_t tmpFill = aFill;
    if((aBase != NumericSystem::cBinary) && (aBase != NumericSystem::cDecimal) && (aBase != NumericSystem::cHexadecimal)) {
      aAppender.push(cNumericError);
      return;
    }
    else { // nothing to do
    }
    if(sConfig->appendBasePrefix && (aBase == NumericSystem::cBinary)) {
      aAppender.push(cNumericFill);
      aAppender.push(cNumericMarkBinary);
    }
    else { // nothing to do
    }
    if(sConfig->appendBasePrefix && (aBase == NumericSystem::cHexadecimal)) {
      aAppender.push(cNumericFill);
      aAppender.push(cNumericMarkHexadecimal);
    }
    else { // nothing to do
    }
    char tmpBuffer[tAppendStackBufferLength];
    uint8_t where = 0u;
    bool negative = aValue < 0;
    do {
      tValueType mod = tmpValue % aBase;
      if(mod < 0) {
        mod = -mod;
      }
      else { // nothing to do
      }
      tmpBuffer[where] = cDigit2char[mod];
      ++where;
      tmpValue /= aBase;
    }
    while((tmpValue != 0) && (where <= tAppendStackBufferLength));
    if(where > tAppendStackBufferLength) {
      aAppender.push(cNumericError);
      return;
    }
    else { // nothing to do
    }
    if(negative) {
      aAppender.push(cMinus);
    }
    else if(sConfig->alignSigned && (aFill > 0u)) {
      aAppender.push(cSpace);
    }
    else { // nothing to do
    }
    if(tmpFill > where) {
      tmpFill -= where;
      while(tmpFill > 0u) {
        aAppender.push(cNumericFill);
        --tmpFill;
      }
    }
    for(--where; where > 0u; --where) {
      aAppender.push(tmpBuffer[where]);
    }
    aAppender.push(tmpBuffer[0]);
  }

  void append(Appender& aAppender, double const aValue, uint8_t const aDigitsNeeded) noexcept {
    if(std::isnan(aValue)) {
      append(aAppender, cNan);
      return;
    } else if(std::isinf(aValue)) {
      append(aAppender, cInf);
      return;
    } else if(aValue == 0.0) {
      aAppender.push(cNumericFill);
      return;
    }
    else {
      double aValue = aValue;
      if(aValue < 0) {
          aValue = -aValue;
          aAppender.push(cMinus);
      }
      else if(sConfig->alignSigned) {
        aAppender.push(cSpace);
      }
      else { // nothing to do
      }
      double mantissa = floor(log10(aValue));
      double normalized = aValue / pow(10.0, mantissa);
      int32_t firstDigit;
      for(uint8_t i = 1u; i < aDigitsNeeded; i++) {
        firstDigit = static_cast<int>(normalized);
        if(firstDigit > 9) {
          firstDigit = 9;
        }
        else { // nothing to do
        }
        aAppender.push(cDigit2char[firstDigit]);
        normalized = 10.0 * (normalized - firstDigit);
        if(i == 1u) {
          aAppender.push(cFractionDot);
        }
        else { // nothing to do
        }
      }
      firstDigit = static_cast<int>(round(normalized));
      if(firstDigit > 9) {
        firstDigit = 9;
      }
      else { // nothing to do
      }
      aAppender.push(cDigit2char[firstDigit]);
      aAppender.push(cScientificE);
      if(mantissa >= 0) {
        aAppender.push(cPlus);
      }
      else { // nothing to do
      }
      append(aAppender, static_cast<int32_t>(mantissa), static_cast<int32_t>(10), 0u);
    }
  }
};// class Log

} // namespace nowtech::log

#endif // NOWTECH_LOG_INCLUDED