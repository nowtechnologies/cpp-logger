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

template<typename tQueue, typename tSender, LogTopic tMaxTopicCount, TaskRepresentation tTaskRepresentation>
class Log final {
private:
  static constexpr bool csSendInBackground = (tQueue::tDirectBufferLength == 0u);
  using tMessage = tQueue::tMessage;
  static constexpr bool csSupport64 = tMessage::tSupport64;
  using tAppInterface = tSender::tAppInterface;
  using tConverter = tSender::tConverter;
  using tLogSize = tQueue::tLogSize;
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

  inline static constexpr char csRegisteredTask[]    = "-=- Registered task: ";
  inline static constexpr char csUnregisteredTask[]  = "-=- Unregistered task: ";
  inline static constexpr char csStringToLogOnDone[] = "\n";
  inline static constexpr char csUnknownTaskName[]   = "UNKNOWN";
  inline static constexpr char csAnonymousTaskName[] = "ANONYMOUS";
  inline static constexpr char csIsrTaskName[]       = "ISR";

  // TODO handle ISR and these custom names

  inline static LogConfig const * sConfig;
  inline static std::atomic<LogTopic> sNextFreeTopic = csFirstFreeTopic;
  inline static Appender* sShiftChainingAppenders;

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
  friend class LogShiftChainHelperBackgroundSend;

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

  // Background sender functions.

  template <typename tDummy = LogShiftChainHelperBackgroundSend>
  static auto i() -> std::enable_if_t<cSendInBackground, tDummy> noexcept {
    TaskIdType const taskId = tAppInterface::getCurrentTaskId();
    return sendHeaderBackground(taskId);
  }

  template <typename tDummy = LogShiftChainHelperBackgroundSend>
  static auto i(TaskId const aTaskId) -> std::enable_if_t<cSendInBackground, tDummy> noexcept {
    return sendHeaderBackground(aTaskId);
  }

  template <typename tDummy = LogShiftChainHelperBackgroundSend>
  static auto i(LogTopic const aTopic) -> std::enable_if_t<cSendInBackground, tDummy> noexcept {
    if(sRegisteredTopics[aTopic] != nullptr) {
      TaskIdType const taskId = tAppInterface::getCurrentTaskId();
      return sendHeaderBackground(taskId, sRegisteredTopics[aTopic]);
    }
    else {
      return sendHeaderBackground(csInvalidTaskId);
    }
  }

  template <typename tDummy = LogShiftChainHelperBackgroundSend>
  static auto i(LogTopic const aTopic, TaskId const aTaskId) -> std::enable_if_t<cSendInBackground, tDummy> noexcept {
    if(sRegisteredTopics[aTopic] != nullptr) {
      return sendHeaderBackground(aTaskId, sRegisteredTopics[aTopic]);
    }
    else {
      return sendHeaderBackground(csInvalidTaskId);
    }
  }

  template <typename tDummy = LogShiftChainHelperBackgroundSend>
  static auto n() -> std::enable_if_t<cSendInBackground, tDummy> noexcept {
    return LogShiftChainHelperBackgroundSend{tAppInterface::getCurrentTaskId()};
  }

  template <typename tDummy = LogShiftChainHelperBackgroundSend>
  static auto n(TaskId const aTaskId) -> std::enable_if_t<cSendInBackground, tDummy> noexcept {
    return LogShiftChainHelperBackgroundSend{aTaskId};
  }

  template <typename tDummy = LogShiftChainHelperBackgroundSend>
  static auto n(LogTopic const aTopic) -> std::enable_if_t<cSendInBackground, tDummy> noexcept {
    if(sRegisteredTopics[aTopic] != nullptr) {
      return LogShiftChainHelperBackgroundSend{tAppInterface::getCurrentTaskId()};
    }
    else {
      return LogShiftChainHelperBackgroundSend{csInvalidTaskId};
    }
  }

  template <typename tDummy = LogShiftChainHelperBackgroundSend>
  static auto n(LogTopic const aTopic, TaskIdType const aTaskId) -> std::enable_if_t<cSendInBackground, tDummy> noexcept {
    if(sRegisteredTopics[aTopic] != nullptr) {
      return LogShiftChainHelperBackgroundSend{aTaskId};
    }
    else {
      return LogShiftChainHelperBackgroundSend{csInvalidTaskId};
    }
  }

  // Now come the direct sender functions.

  

private:
  // Background header creation.

  template <typename tDummy = LogShiftChainHelperBackgroundSend>
  static auto sendHeaderBackground(TaskId const aTaskId) -> std::enable_if_t<cSendInBackground, tDummy> noexcept {
    LogShiftChainHelperBackgroundSend result{aTaskId};
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

  template <typename tDummy = LogShiftChainHelperBackgroundSend>
  static auto sendHeaderBackground(TaskId const aTaskId, char const * aTopicName) -> std::enable_if_t<cSendInBackground, tDummy> noexcept {
    LogShiftChainHelperBackgroundSend result{aTaskId} = sendHeaderBackground(aTaskId);
    if(aTopicName != nullptr && aTopicName[0] != 0) {
      result << sConfig->taskIdFormat;
    }
    else { // nothing to do
    }
    return result;
  }

  // Direct header creation.

};

}

#endif