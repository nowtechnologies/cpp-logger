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

template<typename tQueue, typename tSender, LogTopic tMaxTopicCount, TaskRepresentation tTaskRepresentation>
class Log;

class LogTopicInstance final {
  template<typename tQueue, typename tSender, LogTopic tMaxTopicCount, TaskRepresentation tTaskRepresentation>
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
  inline static constexpr LogFormat B4      { 2u, 4u };
  inline static constexpr LogFormat B8      { 2u, 8u };
  inline static constexpr LogFormat B12     { 2u, 12u};
  inline static constexpr LogFormat B16     { 2u, 16u};
  inline static constexpr LogFormat B24     { 2u, 24u};
  inline static constexpr LogFormat B32     { 2u, 32u};
  inline static constexpr LogFormat D1      {10u,  1u};
  inline static constexpr LogFormat D2      {10u,  2u};
  inline static constexpr LogFormat D3      {10u,  3u};
  inline static constexpr LogFormat D4      {10u,  4u};
  inline static constexpr LogFormat D5      {10u,  5u};
  inline static constexpr LogFormat D6      {10u,  6u};
  inline static constexpr LogFormat D7      {10u,  7u};
  inline static constexpr LogFormat D8      {10u,  8u};
  inline static constexpr LogFormat D16     {10u, 16u};
  inline static constexpr LogFormat X1      {16u,  1u};
  inline static constexpr LogFormat X2      {16u,  2u};
  inline static constexpr LogFormat X3      {16u,  3u};
  inline static constexpr LogFormat X4      {16u,  4u};
  inline static constexpr LogFormat X6      {16u,  6u};
  inline static constexpr LogFormat X8      {16u,  8u};
  inline static constexpr LogFormat X16     {16u, 16u};

  /// If true, task registration will be sent to the output in the form
  /// in the form -=- Registered task: taskname (1) -=-
  bool allowRegistrationLog = true;

  /// Format for displaying the task ID in the message header.
  LogFormat taskIdFormat    = X2;

  /// Format for displaying the FreeRTOS ticks in the header, if any. Should be
  /// LogFormat::cInvalid to disable tick output.
  LogFormat tickFormat      = D5;
  LogFormat defaultFormat   = D5;

  LogConfig() noexcept = default;
};

/// Dummy type to use in << chain as end marker.
enum class LogShiftChainEndMarker : uint8_t {
  cEnd      = 0u
};

template<typename tQueue, typename tSender, LogTopic tMaxTopicCount, TaskRepresentation tTaskRepresentation, size_t tDirectBufferLength>
class Log final {
private:
  static constexpr bool csSendInBackground = (tDirectBufferLength == 0u); // will omit tQueue
  using tMessage = tQueue::tMessage;
  static constexpr bool csSupport64 = tMessage::tSupport64;
  using tAppInterface = tSender::tAppInterface;
  using tConverter = tSender::tConverter;
  using ConversionResult = tConverter::ConversionResult;
  using TopicPrefix = char const *;
  
  static constexpr TaskId  csInvalidTaskId  = tSend::csInvalidTaskId;
  static constexpr TaskId  csMaxTaskIdCount = tSend::csMaxTaskCount + 1u;

  static constexpr LogTopic csFreeTopicIncrement = 1;
  static constexpr LogTopic csFirstFreeTopic = 0;

  static_assert(tMaxTopicCount <= std::numeric_limits<LogTopic>::max());
  static_assert(cInvalidTaskId == std::numeric_limits<TaskId>::max());
  static_assert(tSend::csMaxTaskCount < std::numeric_limits<TaskId>::max() - 1u);
  static_assert(tSend::csMaxTaskCount == csLocalTaskId);

  inline static constexpr char csRegisteredTask[]    = "-=- Registered task: ";
  inline static constexpr char csUnregisteredTask[]  = "-=- Unregistered task: ";
  /*
  TODO these go in tAppInterface.
  inline static constexpr char csUnknownTaskName[]   = "UNKNOWN";
  inline static constexpr char csAnonymousTaskName[] = "ANONYMOUS";
  inline static constexpr char csIsrTaskName[]       = "ISR";

  TODO tAppInterface::getCurrentTaskId() returns 0 if ISR and it is enabled. If disabled, csInvalidTaskId.
  TODO similar is for getTaskName() - if in ISR, returns "ISR" */

  inline static LogConfig const * sConfig;
  inline static std::atomic<LogTopic> sNextFreeTopic = csFirstFreeTopic;

  Log() = delete;

  /// This will be used to send via queue. It stores the first message, and sends it only with the terminal marker.
  class LogShiftChainHelperBackgroundSend final {
    TaskId          mTaskId;
    LogFormat       mNextFormat;
    MessageSequence mNextSequence;
    tMessage        mFirstMessage;

  public:
    LogShiftChainHelperBackgroundSend() noexcept = delete;

    LogShiftChainHelperBackgroundSend(TaskId& aTaskId) noexcept
     : mTaskId(aTaskId)
     , mNextSequence(0u) {
       mNextFormat.invalidate();
    }

    /// Can be used in application code to eliminate further operator<< calls when the topic is disabled.
    bool isValid() const noexcept {
      return mTaskId != csInvalidTaskId;
    }

    template<typename tValue>
    LogShiftChainHelperBackgroundSend& operator<<(tValue const aValue) noexcept {
      if(mTaskId != csInvalidTaskId && mNextSequence < std::numeric_limits<MessageSequence>::max()) {
        LogFormat format;
        if(mNextFormat.isValid()) {
          format = mNextFormat;
        }
        else {
          format = sConfig->defaultFormat;
        }
        tMessage message;
        message.set(aValue, format, mTaskId, mNextSequence);
        if(mNextSequence == 0u) {
          mFirstMessage = message;
        }
        else {
          tQueue::push(message);
        }
        mNextFormat.invalidate();
        ++mNextSequence;
      }
      else { // silently discard value, nothing to do
      }
      return *this;
    }

    LogShiftChainHelperBackgroundSend& operator<<(LogFormat const aFormat) noexcept {
      mNextFormat = aFormat;
      return *this;
    }

    void operator<<(LogShiftChainEndMarker const) noexcept {
      if(mTaskId != csInvalidTaskId) {
        tQueue::push(mFirstMessage);
      }
      else { // nothing to do
      }
    }
  }; // class LogShiftChainHelperBackgroundSend

  /// This will be used to send directly, blocking the current thread.
  class LogShiftChainHelperDirectSend final {
    TaskId          mTaskId;
    LogFormat       mNextFormat;

  public:
    LogShiftChainHelperDirectSend() noexcept = delete;

    LogShiftChainHelperDirectSend(TaskId& aTaskId) noexcept
     : mTaskId(aTaskId) {
       mNextFormat.invalidate();
    }

    /// Can be used in application code to eliminate further operator<< calls when the topic is disabled.
    bool isValid() const noexcept {
      return mTaskId != csInvalidTaskId;
    }

    template<typename tValue>
    LogShiftChainHelperDirectSend& operator<<(tValue const aValue) noexcept {
      if(mTaskId != csInvalidTaskId) {
        LogFormat format;
        if(mNextFormat.isValid()) {
          format = mNextFormat;
        }
        else {
          format = sConfig->defaultFormat;
        }
        ConversionResult buffer[tDirectBufferLength];
        tConverter converter(buffer, buffer + tDirectBufferLength);
        converter.convert(aValue);
        tAppInterface::lock();
        tSender::send(buffer, converter.end());
        tAppInterface::unlock();
      }
      else { // silently discard value, nothing to do
      }
      return *this;
    }

    LogShiftChainHelperDirectSend& operator<<(LogFormat const aFormat) noexcept {
      mNextFormat = aFormat;
      return *this;
    }

    void operator<<(LogShiftChainEndMarker const) noexcept {
      if(mTaskId != csInvalidTaskId) {
        ConversionResult buffer[tDirectBufferLength];
        tConverter converter(buffer, buffer + tDirectBufferLength);
        converter.terminateSequence();
        tAppInterface::lock();
        tSender::send(buffer, converter.end());
        tAppInterface::unlock();
      }
      else { // nothing to do
      }
    }
  }; // class LogShiftChainHelperDirectSend

  using LogShiftChainHelper = std::conditional_t<csSendInBackground, LogShiftChainHelperBackgroundSend, LogShiftChainHelperDirectSend>;

public:
  /// Will be used as Log << something << to << log << Log::end;
  static constexpr LogShiftChainEndMarker end = LogShiftChainEndMarker::cEnd;

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
    sRegisteredTopics = tAppInterface::template _newArray<TopicPrefix>(tMaxTopicCount);
    std::fill_n(sRegisteredTopics, tMaxTopicCount, nullptr);
  }

  // TODO note in docs about init and done sequence
  static void done() {
    tQueue::done();
    tSender::done();
    tAppInterface::done();
    tAppInterface::template _deleteArray<TopicPrefix>(sRegisteredTopics);
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
        n() << csRegisteredTask << aTaskName << taskId << end;
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
      n() << csRegisteredTask << tAppInterface::getTaskName(taskId) << taskId << end;
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

  static auto i() noexcept {
    TaskIdType const taskId = tAppInterface::getCurrentTaskId();
    return sendHeader(taskId);
  }

  static LogShiftChainHelper i(TaskId const aTaskId) noexcept {
    return sendHeader(aTaskId);
  }

  static LogShiftChainHelper i(LogTopic const aTopic) noexcept {
    if(sRegisteredTopics[aTopic] != nullptr) {
      TaskIdType const taskId = tAppInterface::getCurrentTaskId();
      return sendHeader(taskId, sRegisteredTopics[aTopic]);
    }
    else {
      return sendHeader(csInvalidTaskId);
    }
  }

  static LogShiftChainHelper i(LogTopic const aTopic, TaskId const aTaskId) noexcept {
    if(sRegisteredTopics[aTopic] != nullptr) {
      return sendHeader(aTaskId, sRegisteredTopics[aTopic]);
    }
    else {
      return sendHeader(csInvalidTaskId);
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
  static LogShiftChainHelper sendHeader(TaskId const aTaskId) noexcept {
    LogShiftChainHelper result{aTaskId};
    if(result.isValid()) {
      if constexpr(tTaskRepresentation == TaskRepresentation::cId) {
        result << sConfig->taskIdFormat << aTaskId;
      }
      else if constexpr (tTaskRepresentation == TaskRepresentation::cName) {
        result << tAppInterface::getTaskName(aTaskId);
      }
      else { // nothing to do
      }
      if (sConfig->tickFormat.isValid()) {
        result << sConfig->tickFormat << tAppInterface::getLogTime();
      }
      else { // nothing to do
      }
    }
    else { // nothing to do
    }
    return result;
  }

  static LogShiftChainHelper sendHeader(TaskId const aTaskId, char const * aTopicName) noexcept {
    LogShiftChainHelper result{aTaskId} = sendHeader(aTaskId);
    if(aTopicName != nullptr && aTopicName[0] != 0) {
      result << sConfig->taskIdFormat;
    }
    else { // nothing to do
    }
    return result;
  }

};

}

#endif