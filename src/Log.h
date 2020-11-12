#ifndef NOWTECH_LOG
#define NOWTECH_LOG

#include "LogMessageBase.h"
#include <type_traits>
#include <algorithm>
#include <atomic>
#include <limits>

namespace nowtech::log {
  
enum class Exception : uint8_t {
  cOutOfTaskIdsOrDoubleRegistration = 0u,
  cOutOfTopics                      = 1u
};

enum class TaskRepresentation : uint8_t {
  cNone = 0u,
  cId   = 1u,
  cName = 2u
};

typedef int8_t LogTopic; // this needs to be signed to let the overload resolution work

template<typename tQueue, typename tSender, size_t tDirectBufferLength, LogTopic tMaxTopicCount, uint8_t tAppendStackBufferLength, TaskRepresentation tTaskRepresentation, bool tAppendBasePrefix, bool tAlignSigned>
class Log;

class LogTopicInstance final {
  template<typename tQueue, typename tSender, size_t tDirectBufferLength, LogTopic tMaxTopicCount, uint8_t tAppendStackBufferLength, TaskRepresentation tTaskRepresentation, bool tAppendBasePrefix, bool tAlignSigned>
  friend class Log;

public:
  static constexpr LogTopic cInvalidTopic = std::numeric_limits<LogTopic>::min();

private:
  LogTopic mValue = cInvalidTopic;

  LogTopic operator=(LogTopic const aValue) {
    mValue = aValue;
    return aValue;
  }

public:
  LogTopic operator*() {
    return mValue;
  }

  operator LogTopic() {
    return mValue;
  }
};

struct LogConfig final {
public:
  /// This is the default logging format and the only one I will document
  /// here. For the others, the letter represents the base of the number
  /// system and the number represents the minimum digits to write, possibly
  /// with leading zeros. When formats are applied to floating point
  /// numbers, the numeric system info is discarded.
  inline static constexpr LogFormatEnd cDefault = {10u, 0u };
  inline static constexpr LogFormatEnd cInvalid = { 0u, 0u };
  inline static constexpr LogFormatEnd cB4      = { 2u, 4u };
  inline static constexpr LogFormatEnd cB8      = { 2u, 8u };
  inline static constexpr LogFormatEnd cB12     = { 2u, 12u};
  inline static constexpr LogFormatEnd cB16     = { 2u, 16u};
  inline static constexpr LogFormatEnd cB24     = { 2u, 24u};
  inline static constexpr LogFormatEnd cB32     = { 2u, 32u};
  inline static constexpr LogFormatEnd cD1      = {10u,  1u};
  inline static constexpr LogFormatEnd cD2      = {10u,  2u};
  inline static constexpr LogFormatEnd cD3      = {10u,  3u};
  inline static constexpr LogFormatEnd cD4      = {10u,  4u};
  inline static constexpr LogFormatEnd cD5      = {10u,  5u};
  inline static constexpr LogFormatEnd cD6      = {10u,  6u};
  inline static constexpr LogFormatEnd cD7      = {10u,  7u};
  inline static constexpr LogFormatEnd cD8      = {10u,  8u};
  inline static constexpr LogFormatEnd cD16     = {10u, 16u};
  inline static constexpr LogFormatEnd cX1      = {16u,  1u};
  inline static constexpr LogFormatEnd cX2      = {16u,  2u};
  inline static constexpr LogFormatEnd cX3      = {16u,  3u};
  inline static constexpr LogFormatEnd cX4      = {16u,  4u};
  inline static constexpr LogFormatEnd cX6      = {16u,  6u};
  inline static constexpr LogFormatEnd cX8      = {16u,  8u};
  inline static constexpr LogFormatEnd cX16     = {16u, 16u};

  /// If true, task registration will be sent to the output in the form
  /// in the form -=- Registered task: taskname (1) -=-
  bool allowRegistrationLog = true;

  /// Format for displaying the task ID in the message header.
  LogFormatEnd taskIdFormat = cX2;

  /// Format for displaying the FreeRTOS ticks in the header, if any. Should be
  /// LogFormatEnd::cInvalid to disable tick output.
  LogFormatEnd tickFormat   = cD5;

  /// These are default formats for some types.
  LogFormatEnd int8Format       = cDefault;
  LogFormatEnd int16Format      = cDefault;
  LogFormatEnd int32Format      = cDefault;
  LogFormatEnd int64Format      = cDefault;
  LogFormatEnd uint8Format      = cDefault;
  LogFormatEnd uint16Format     = cDefault;
  LogFormatEnd uint32Format     = cDefault;
  LogFormatEnd uint64Format     = cDefault;
  LogFormatEnd floatFormat      = cD5;
  LogFormatEnd doubleFormat     = cD8;
  LogFormatEnd longDoubleFormat = cD16;

  LogConfig() noexcept = default;
};

/// Dummy type to use in << chain as end marker.
enum class LogShiftChainMarker : uint8_t {
  cEnd      = 0u
};

template<typename tQueue, typename tSender, size_t tDirectBufferLength, LogTopic tMaxTopicCount, uint8_t tAppendStackBufferLength, TaskRepresentation tTaskRepresentation, bool tAppendBasePrefix, bool tAlignSigned>
class Log final {
private:
  using tMessage = tQueue::tMessage;
  using tAppInterface = tSender::tAppInterface;
  using cSupport64 = tMessage::tSupport64;
  using LogSize = tQueue::LogSize;
  using cSendInBackground = tQueue::cSendInBackground;
  using IntegerConversionUnsigned = std::conditional_t<Support64, uint64_t, uint32_t>;
  using IntegerConversionSigned = std::conditional_t<Support64, int64_t, int32_t>;
  using TopicPrefix = char const *;
  
  static constexpr TaskId  csInvalidTaskId  = tSend::csInvalidTaskId;
  static constexpr TaskId  csMaxTaskIdCount = tSend::csMaxTaskCount + 1u;

  static constexpr LogTopic csFreeTopicIncrement = 1;
  static constexpr LogTopic csFirstFreeTopic = 0;

  static_assert(tMaxTopicCount <= std::numeric_limits<LogTopic>::max());
  static_assert(std::is_unsigned_v<LogSize>);
  static_assert(cInvalidTaskId == std::numeric_limits<TaskId>::max());
  static_assert(tSend::csMaxTaskCount < std::numeric_limits<TaskId>::max() - 1u);
  static_assert(tSend::csMaxTaskCount == csLocalTaskId);

  static constexpr char csNumericError            = '#';

  static constexpr char csEndOfMessage            = '\r';
  static constexpr char csEndOfLine               = '\n';
  static constexpr char csNumericFill             = '0';
  static constexpr char csNumericMarkBinary       = 'b';
  static constexpr char csNumericMarkHexadecimal  = 'x';
  static constexpr char csMinus                   = '-';
  static constexpr char csSpace                   = ' ';
  static constexpr char csSeparatorFailure        = '@';
  static constexpr char csFractionDot             = '.';
  static constexpr char csPlus                    = '+';
  static constexpr char csScientificE             = 'e';


  inline static constexpr char csNan[]               = "nan";
  inline static constexpr char csInf[]               = "inf";
  inline static constexpr char csRegisteredTask[]    = "-=- Registered task: ";
  inline static constexpr char csUnregisteredTask[]  = "-=- Unregistered task: ";
  inline static constexpr char csStringToLogOnDone[] = "\n";
  inline static constexpr char csUnknownTaskName[]   = "UNKNOWN";
  inline static constexpr char csAnonymousTaskName[] = "ANONYMOUS";
  inline static constexpr char csIsrTaskName[]       = "ISR";


  inline static constexpr char csDigit2char[static_cast<uint8_t>(NumericSystem::cHexadecimal)] = {
    '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
  };

  inline static LogConfig const * sConfig;
  inline static std::atomic<LogTopic> sNextFreeTopic = csFirstFreeTopic;
  inline static Appender* sShiftChainingAppenders;

  Log() = delete;

  class LogShiftChainHelper final {
    TaskId          mTaskId;
    LogFormatEnd    mNextFormat;
    MessageSequence mNextSequence;

  public:
    LogShiftChainHelper() noexcept = delete;

    LogShiftChainHelper(TaskId& aTaskId) noexcept
     : mTaskId(aTaskId)
     , mNextSequence(0u) {
    }

    LogShiftChainHelper(TaskId& aTaskId, MessageSequence aNextSequence) noexcept
     : mTaskId(aTaskId) 
     , mNextSequence(aNextSequence){
    }

    bool isValid() const noexcept {
      return mTaskId != csInvalidTaskId;
    }

    template<typename tValue>
    LogShiftChainHelper& operator<<(tValue const aValue) noexcept {
      if(mTaskId != csInvalidTaskId && mNextSequence < std::numeric_limits<MessageSequence>::max()) {
        tMessage message;
        message.set(aValue, mNextFormatEnd, mTaskId, mNextSequence);
        tQueue::push(message);
        mNextFormat.invalidate();
        ++mNextSequence;
      }
      else { // silently discard value, nothing to do
      }
      return *this;
    }

    LogShiftChainHelper& operator<<(LogFormatEnd const aFormat) noexcept {
      mNextFormat = aFormat;
      return *this;
    }

    void operator<<(LogShiftChainMarker const) noexcept {
      if(mTaskId != csInvalidTaskId) {
        tMessage message;
        message.invalidate(mNextSequence);
        tQueue::push(message);
      }
      else { // nothing to do
      }
    }
  }; // class LogShiftChainHelper
  friend class LogShiftChainHelper;

public:
  /// Will be used as Log << something << to << log << Log::end;
  static constexpr LogShiftChainMarker end = LogShiftChainMarker::cEnd;

  // TODO remark in docs: must come before registering topics
  static void init(LogConfig const &aConfig) {
    sConfig = &aConfig;
    if constexpr(cSendInBackground) {
      tAppInterface::init([](){ transmitterTaskFunction(); });
    }
    else {
      tAppInterface::init();
    }
    tSender::init();
    tQueue::init();
    sShiftChainingAppenders = tAppInterface::template _newArray<Appender>(cMaxTaskIdCount);
    sRegisteredTopics = tAppInterface::template _newArray<TopicPrefix>(tMaxTopicCount);
    std::fill_n(sRegisteredTopics, tMaxTopicCount, nullptr);
  }

  // TODO note in docs about init and done sequence
  static void done() {
    tQueue::done();
    tSender::done();
    tAppInterface::done();
    tAppInterface::template _deleteArray<Appender>(sShiftChainingAppenders);
    tAppInterface::template _deleteArray<Appender>(sRegisteredTopics);
  }

  /// Registers the current task if not already present. It can register
  /// at most 254 tasks. All others will be handled as one.
  /// NOTE: this method locks to inhibit concurrent access of methods with the same name.
  static void registerCurrentTask() noexcept {
    registerCurrentTask(nullptr);
  }

  /// Registers the current task if not already present. It can register
  /// at most 254 tasks. All others will be handled as one.
  /// @param aTaskName Task name to use, when the osInterface supports it.
  static void registerCurrentTask(char const * const aTaskName) {
    TaskId taskId = tAppInterface::registerCurrentTask(aTaskName);
    if(taskId != csInvalidTaskId) {
      if(sConfig->allowRegistrationLog) {
        // TODO log it
      }
      else { // nothing to do
      }
    }
    else {
      tAppInterface::fatalError(Exception::cOutOfTaskIdsOrDoubleRegistration);
    }
  }

  static void unregisterCurrentTask() noexcept {
    TaskId taskId = tAppInterface::unregisterCurrentTask();
    if(taskId != cInvalidTaskId && sConfig->allowRegistrationLog) {
      // TODO log it
    }
    else { // nothing to do
    }
  }

  static void registerTopic(LogTopicInstance &aTopic, char const * const aPrefix) {
    aTopic = sNextFreeTopic.fetch_add(cFreeTopicIncrement);
    if(aTopic >= tMaxTopicCount) {
      tAppInterface::fatalError(Exception::cOutOfTopics);
    }
    else {
      sRegisteredTopics[aTopic] = aPrefix;
    }
  }
  
  static [[nodiscard]] TaskId getCurrentTaskId() noexcept {   // nodiscard because possibly expensive call
    return tAppInterface::getCurrentTaskId(cLocalTaskId);
  }

  static LogShiftChainHelper i() noexcept {
    TaskIdType const taskId = tAppInterface::getCurrentTaskId();
    MessageSequence next = sendHeader(taskId);
    return LogShiftChainHelper{taskId, next};
  }

  static LogShiftChainHelper i(TaskId const aTaskId) noexcept {
    MessageSequence next = sendHeader(aTaskId);
    return LogShiftChainHelper{aTaskId, next};
  }

  static LogShiftChainHelper i(LogTopic const aTopic) noexcept {
    if(sRegisteredTopics[aTopic] != nullptr) {
      TaskIdType const taskId = tAppInterface::getCurrentTaskId();
      MessageSequence next = sendHeader(taskId, sRegisteredTopics[aTopic]);
      return LogShiftChainHelper{taskId, next};
    }
    else {
      return LogShiftChainHelper{csInvalidTaskId};
    }
  }

  static LogShiftChainHelper i(LogTopic const aTopic, TaskId const aTaskId) noexcept {
    if(sRegisteredTopics[aTopic] != nullptr) {
      MessageSequence next = sendHeader(aTaskId, sRegisteredTopics[aTopic]);
      return LogShiftChainHelper{aTaskId, next};
    }
    else {
      return LogShiftChainHelper{csInvalidTaskId};
    }
  }

  static LogShiftChainHelper n() noexcept {
    return LogShiftChainHelper{tAppInterface::getCurrentTaskId()};
  }

  static LogShiftChainHelper n(TaskId const aTaskId) noexcept {
    return LogShiftChainHelper{aTaskId};
  }

  static LogShiftChainHelper n(LogTopic const aTopic) noexcept {
    if(sRegisteredTopics[aTopic] != nullptr) {
      return LogShiftChainHelper{tAppInterface::getCurrentTaskId()};
    }
    else {
      return LogShiftChainHelper{csInvalidTaskId};
    }
  }

  static LogShiftChainHelper n(LogTopic const aTopic, TaskIdType const aTaskId) noexcept {
    if(sRegisteredTopics[aTopic] != nullptr) {
      return LogShiftChainHelper{aTaskId};
    }
    else {
      return LogShiftChainHelper{csInvalidTaskId};
    }
  }

private:
  static MessageSequence sendHeader(TaskId const aTaskId) noexcept {
    MessageSequence next = 0u;
    if constexpr(tTaskRepresentation == TaskRepresentation::cId) {
      tMessage message;
      message.set(aTaskId, sConfig->taskIdFormat, aTaskId, next);
      tQueue::push(message);
      ++next;  
    }
    else if constexpr (tTaskRepresentation == TaskRepresentation::cName) {
      tMessage message;
      message.set(tAppInterface::getTaskName(aTaskId), sConfig->taskIdFormat, aTaskId, next);
      tQueue::push(message);
      ++next;
    }
    else { // nothing to do
    }
    if (sConfig->tickFormat.isValid()) {
      tMessage message;
      message.set(tAppInterface::getLogTime(), sConfig->tickFormat, aTaskId, next);
      tQueue::push(message);
      ++next;
    }
    else { // nothing to do
    }
    return next;
  }

  static MessageSequence sendHeader(TaskId const aTaskId, char const * aTopicName) noexcept {
    MessageSequence next = sendHeader(aTaskId);
    if(aTopicName != nullptr && aTopicName[0] != 0) {
      tMessage message;
      message.set(aTopicName, sConfig->taskIdFormat, aTaskId, next);
      tQueue::push(message);
      ++next;
    }
    else { // nothing to do
    }
    return next;
  }
};

}

#endif