#ifndef NOWTECH_LOG
#define NOWTECH_LOG

#include "LogMessageBase.h"
#include "PoolAllocator.h"
#include <type_traits>
#include <algorithm>
#include <atomic>
#include <limits>
#include <array>

namespace nowtech::log {
  
enum class Exception : uint8_t {
  cOutOfTaskIdsOrDoubleRegistration = 0u,
  cOutOfTopics                      = 1u,
  cSenderError                      = 2u,
  cCount                            = 3u
};

enum class TaskRepresentation : uint8_t {
  cNone = 0u,
  cId   = 1u,
  cName = 2u
};

using LogTopic = int8_t; // this needs to be signed to let the overload resolution work

template<typename tQueue, typename tSender, LogTopic tMaxTopicCount, TaskRepresentation tTaskRepresentation, size_t tDirectBufferSize, typename tSender::tAppInterface_::LogTime tRefreshPeriod>
class Log;

class TopicInstance final {
  template<typename tQueue, typename tSender, LogTopic tMaxTopicCount, TaskRepresentation tTaskRepresentation, size_t tDirectBufferSize, typename tSender::tAppInterface_::LogTime tRefreshPeriod>
  friend class Log;

public:
  static constexpr LogTopic csInvalidTopic = std::numeric_limits<LogTopic>::min();

private:
  LogTopic mValue = csInvalidTopic;

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
  inline static constexpr LogFormat B4      { 2u,  4u};
  inline static constexpr LogFormat B8      { 2u,  8u};
  inline static constexpr LogFormat B12     { 2u, 12u};
  inline static constexpr LogFormat B16     { 2u, 16u};
  inline static constexpr LogFormat B24     { 2u, 24u};
  inline static constexpr LogFormat B32     { 2u, 32u};
  inline static constexpr LogFormat Fm      {10u,  0u};
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

  // Indicates the next char* value should be stored in messages instead of taking only its address
  inline static constexpr LogFormat St      {13u, LogFormat::csStoreStringFillValue };

  /// If true, task registration will be sent to the output in the form
  /// in the form -=- Registered task: taskname (1) -=-
  bool allowRegistrationLog = true;

  /// Format for displaying the task ID in the message header.
  LogFormat taskIdFormat    = X2;

  /// Format for displaying the FreeRTOS ticks in the header, if any. Should be
  /// LogFormat::cInvalid to disable tick output.
  LogFormat tickFormat      = D5;
  LogFormat defaultFormat   = Fm;

  LogConfig() noexcept = default;
};

/// Dummy type to use in << chain as end marker.
enum class LogShiftChainEndMarker : uint8_t {
  cEnd      = 0u
};

template<typename tQueue, typename tSender, LogTopic tMaxTopicCount, TaskRepresentation tTaskRepresentation, size_t tDirectBufferSize, typename tSender::tAppInterface_::LogTime tRefreshPeriod>
class Log final {
private:
  static constexpr bool csShutDownLog      = tSender::csVoid;
  static constexpr bool csSendInBackground = (tDirectBufferSize == 0u); // will omit tQueue
  using tMessage = typename tQueue::tMessage_;
  using tAppInterface = typename tSender::tAppInterface_;
  using tConverter = typename tSender::tConverter_;
  using ConversionResult = typename tConverter::ConversionResult;
  using LogTime = typename tAppInterface::LogTime;
  using TopicName = char const *;

  // TODO assert two tMessages are identical
  static constexpr size_t   csPayloadSizeBr     = tMessage::csPayloadSize;
  static constexpr size_t   csPayloadSizeNet    = tMessage::csPayloadSize - 1u;  // we leave space for terminal 0 to avoid counting bytes
  static constexpr size_t   csQueueSize         = tQueue::csQueueSize;
  static constexpr TaskId   csInvalidTaskId     = tAppInterface::csInvalidTaskId;
  static constexpr TaskId   csIsrTaskId         = tAppInterface::csIsrTaskId;
  static constexpr TaskId   csMaxTaskCount      = tAppInterface::csMaxTaskCount;
  static constexpr TaskId   csMaxTotalTaskCount = tAppInterface::csMaxTaskCount + 1u;
  static constexpr size_t   csListItemOverhead  = sizeof(void*) * 8u;
  
  static constexpr LogTopic csFirstFreeTopic    = 0;
  static constexpr MessageSequence csSequence0  = 0u;
  static constexpr MessageSequence csSequence1  = 1u;
  static constexpr char csTerminalChar          = 0;

  using Occupier = typename tAppInterface::Occupier;
  using Allocator = memory::PoolAllocator<tMessage, Occupier>;
  using MessageQueue = std::list<tMessage, Allocator>;
  using MessageQueueArray = std::array<MessageQueue*, csMaxTotalTaskCount>; // Need the indirection to be able use allocator in constructor call.
  // Could introduce a new list type but the performance gain would be less than a percent.

  static_assert(csPayloadSizeNet > 0u);
  static_assert(csInvalidTaskId == std::numeric_limits<TaskId>::max());
  static_assert(csIsrTaskId == std::numeric_limits<TaskId>::min());
  static_assert(csMaxTaskCount < std::numeric_limits<TaskId>::max());
  static_assert(std::is_same_v<tAppInterface, typename tQueue::tAppInterface_>);

  inline static constexpr char csRegisteredTask[]    = "-=- Registered task:";
  inline static constexpr char csUnregisteredTask[]  = "-=- Unregistered task:";
  
  inline static LogConfig const * sConfig;
  inline static std::atomic<LogTopic> sNextFreeTopic;
  inline static std::atomic<bool> sKeepAliveTask;
  inline static std::array<TopicName, tMaxTopicCount> sRegisteredTopics;

  inline static Occupier           sOccupier;
  inline static Allocator         *sAllocator;
  inline static MessageQueueArray *sMessageQueues;

  Log() = delete;

  /// This will be used to send via queue. It stores the first message, and sends it only with the terminal marker.
  class LogShiftChainHelperBackgroundSend final {
    TaskId          mTaskId;
    LogFormat       mNextFormat;
    MessageSequence mNextSequence;
    tMessage        mFirstMessage;

  public:
    static constexpr LogShiftChainEndMarker end = LogShiftChainEndMarker::cEnd;

    LogShiftChainHelperBackgroundSend() noexcept = delete;

    LogShiftChainHelperBackgroundSend(TaskId const aTaskId) noexcept
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
          mNextFormat.invalidate();
        }
        else {
          format = sConfig->defaultFormat;
        }
        tMessage message;
        message.set(aValue, format, mTaskId, mNextSequence);
        sendOrStore(message);
      }
      else { // silently discard value, nothing to do
      }
      return *this;
    }

    template<>
    LogShiftChainHelperBackgroundSend& operator<<<char *>(char * aValue) noexcept {
      return sendCharPointer(aValue);
    }

    template<>
    LogShiftChainHelperBackgroundSend& operator<<<char const *>(char const * aValue) noexcept {
      return sendCharPointer(aValue);
    }

    template<>
    LogShiftChainHelperBackgroundSend& operator<<<char * const>(char * const aValue) noexcept {
      return sendCharPointer(aValue);
    }

    template<>
    LogShiftChainHelperBackgroundSend& operator<<<char const * const>(char const * const aValue) noexcept {
      return sendCharPointer(aValue);
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

  private:
    LogShiftChainHelperBackgroundSend& sendCharPointer(char const * const aValue) noexcept {
      if(mTaskId != csInvalidTaskId && mNextSequence < std::numeric_limits<MessageSequence>::max()) {
        LogFormat format;
        if(mNextFormat.isValid()) {
          format = mNextFormat;
          mNextFormat.invalidate();
        }
        else {
          format = sConfig->defaultFormat;
        }
        tMessage message;
        if(format.isStoredString()) {
          std::array<char, csPayloadSizeBr> payload;
          char const * where = aValue;
          while(*where != csTerminalChar) {
            size_t copied = 0u;
            while(*where != csTerminalChar && copied < csPayloadSizeNet) {
              payload[copied] = *where;
              ++where;
              ++copied;
            }
            payload[copied] = csTerminalChar;
            message.set(payload, format, mTaskId, mNextSequence);
            sendOrStore(message);
          }
        }
        else {
          message.set(aValue, format, mTaskId, mNextSequence);
          sendOrStore(message);
        }
      }
      else { // silently discard value, nothing to do
      }
      return *this;
    }

    void sendOrStore(tMessage const & aMessage) noexcept {
      if(mNextSequence == csSequence0) {
        mFirstMessage = aMessage;
      }
      else {
        tQueue::push(aMessage);
      }
      ++mNextSequence;
    }
  }; // class LogShiftChainHelperBackgroundSend

  /// This will be used to send directly, blocking the current thread.
  class LogShiftChainHelperDirectSend final {
    TaskId          mTaskId;
    LogFormat       mNextFormat;

  public:
    LogShiftChainHelperDirectSend() noexcept = delete;

    LogShiftChainHelperDirectSend(TaskId const aTaskId) noexcept
     : mTaskId(aTaskId) {
       mNextFormat.invalidate();
    }

    /// Can be used in application code to eliminate further operator<< calls when the topic is disabled.
    bool isValid() const noexcept {
      return mTaskId != csInvalidTaskId;
    }

    template<typename tValue>
    LogShiftChainHelperDirectSend& operator<<(tValue const aValue) {
      if(mTaskId != csInvalidTaskId) {
        LogFormat format;
        if(mNextFormat.isValid()) {
          format = mNextFormat;
          mNextFormat.invalidate();
        }
        else {
          format = sConfig->defaultFormat;
        }
        ConversionResult buffer[tDirectBufferSize];
        tConverter converter(buffer, buffer + tDirectBufferSize);
        converter.convert(aValue, format.mBase, format.mFill);
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
        ConversionResult buffer[tDirectBufferSize];
        tConverter converter(buffer, buffer + tDirectBufferSize);
        converter.terminateSequence();
        tAppInterface::lock();
        tSender::send(buffer, converter.end());
        tAppInterface::unlock();
      }
      else { // nothing to do
      }
    }
  }; // class LogShiftChainHelperDirectSend

  /// This shuts down all logging code generation without uncommenting anything from user code, when compiled with aT least -O1 
  class LogShiftChainHelperEmpty final {
  public:
    LogShiftChainHelperEmpty() noexcept = delete;

    LogShiftChainHelperEmpty(TaskId const) noexcept {
    }

    /// Can be used in application code to eliminate further operator<< calls when the topic is disabled.
    bool isValid() const noexcept {
      return false;
    }

    template<typename tValue>
    LogShiftChainHelperEmpty& operator<<(tValue const) noexcept {
      return *this;
    }

    LogShiftChainHelperEmpty& operator<<(LogFormat const) noexcept {
      return *this;
    }

    void operator<<(LogShiftChainEndMarker const) noexcept {
    }
  }; // class LogShiftChainHelperEmpty

  using LogShiftChainHelper = std::conditional_t<csShutDownLog, LogShiftChainHelperEmpty, std::conditional_t<csSendInBackground, LogShiftChainHelperBackgroundSend, LogShiftChainHelperDirectSend>>;

public:
  /// Will be used as Log << something << to << log << Log::end;
  static constexpr LogShiftChainEndMarker end = LogShiftChainEndMarker::cEnd;

  // TODO remark in docs: must come before registering topics
  static void init(LogConfig const &aConfig) {
    sConfig = &aConfig;
    if constexpr(csSendInBackground) {
      std::byte experiment[sizeof(tMessage) + csListItemOverhead];
      tMessage example;
      size_t nodeSize = memory::AllocatorBlockGauge<std::list<tMessage>>::getNodeSize(experiment, example);
      sAllocator = tAppInterface::template _new<Allocator>(csQueueSize, nodeSize, sOccupier);
      sMessageQueues = tAppInterface::template _new<MessageQueueArray>();
      auto &messageQueues = *sMessageQueues;
      for(size_t i = 0; i < csMaxTotalTaskCount; ++i) {
        messageQueues[i] = tAppInterface::template _new<MessageQueue>(*sAllocator);
      }
      sKeepAliveTask = true;
      tAppInterface::init([](){ transmitterTaskFunction(); });
    }
    else {
      tAppInterface::init();
    }
    tQueue::init();
    sNextFreeTopic = csFirstFreeTopic;
    std::fill_n(sRegisteredTopics.begin(), tMaxTopicCount, nullptr);
  }

  // TODO note in docs about init and done sequence
  static void done() {
    if constexpr(csSendInBackground) {
      sKeepAliveTask = false;
      tAppInterface::waitForFinished();
      auto &messageQueues = *sMessageQueues;
      for(size_t i = 0; i < tMaxTopicCount; ++i) {
        tAppInterface::template _delete<MessageQueue>(messageQueues[i]);
      }
      tAppInterface::template _delete<MessageQueueArray>(sMessageQueues);
      tAppInterface::template _delete<Allocator>(sAllocator);
    }
    else { // nothing to do
    }
    tQueue::done();
    tSender::done();
    tAppInterface::done();
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
        n(taskId) << csRegisteredTask << aTaskName << taskId << end;
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
    if(taskId != csInvalidTaskId && sConfig->allowRegistrationLog) {
      n(taskId) << csUnregisteredTask << taskId << end;
    }
    else { // nothing to do
    }
  }

  static void registerTopic(TopicInstance &aTopic, char const * const aPrefix) {
    aTopic = sNextFreeTopic++;
    if(aTopic >= tMaxTopicCount) {
      tAppInterface::fatalError(Exception::cOutOfTopics);
    }
    else {
      sRegisteredTopics[aTopic] = aPrefix;
    }
  }
  
  static TaskId getCurrentTaskId() noexcept {
    return tAppInterface::getCurrentTaskId();
  }

  static auto i() noexcept {
    TaskId const taskId = tAppInterface::getCurrentTaskId();
    return sendHeader(taskId);
  }

  static LogShiftChainHelper i(TaskId const aTaskId) noexcept {
    return sendHeader(aTaskId);
  }

  static LogShiftChainHelper i(LogTopic const aTopic) noexcept {
    if(sRegisteredTopics[aTopic] != nullptr) {
      TaskId const taskId = tAppInterface::getCurrentTaskId();
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

  static LogShiftChainHelper n(LogTopic const aTopic, TaskId const aTaskId) noexcept {
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
    LogShiftChainHelper result = sendHeader(aTaskId);
    if(result.isValid() && aTopicName != nullptr) {
      result << aTopicName;
    }
    else { // nothing to do
    }
    return result;
  }

  static void transmitterTaskFunction() noexcept {
    while(sKeepAliveTask || !tQueue::empty()) {
      tMessage message;
      if(tQueue::pop(message, tRefreshPeriod)) {
        auto list = sMessageQueues->at(message.getTaskId());
        bool ready = checkAndInsert(*list, message);
        if(ready) {
          transmit(*list);
        }
        else { // nothing to do
        }
      }
      else { // nothing to do
      }
    }
    tAppInterface::finish();
  }

  // Returns true if the list is ready for transmission.
  static bool checkAndInsert(MessageQueue &aList, tMessage const &aMessage) noexcept {
    bool result = false;
    auto sequence = aMessage.getMessageSequence();
    if(aList.empty()) {
      if(sequence <= csSequence1 && sAllocator->hasFree()) {
        result = push(aList, aMessage, sequence);
      }
      else { // nothing to do
      }
    }
    else {
      auto lastSequence = aList.back().getMessageSequence();
      if((sequence == csSequence0 || sequence == lastSequence + 1u) && sAllocator->hasFree()) {
        result = push(aList, aMessage, sequence);
      }
      else {
        aList.clear();
      }
    }
    return result;
  }

  static bool push(MessageQueue &aList, tMessage const &aMessage, MessageSequence const aSequence) noexcept {
    bool result;
    if(aSequence == csSequence0) {
      aList.push_front(aMessage);
      result = true;
    }
    else {
      aList.push_back(aMessage);
      result = false;
    }
    return result;
  }

  static void transmit(MessageQueue &aList) noexcept {
    auto [begin, end] = tSender::getBuffer();
    tConverter converter(begin, end);
    for(auto &message : aList) {
      message.template output<tConverter>(converter);
    }
    aList.clear();
    converter.terminateSequence();
    tSender::send(begin, converter.end()); // TODO postpone
  }
};

}

using LC = nowtech::log::LogConfig;

#endif