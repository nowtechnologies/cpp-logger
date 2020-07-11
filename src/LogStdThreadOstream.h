//
// Copyright 2018 Now Technologies Zrt.
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

#ifndef NOWTECH_LOG_STD_THREAD_OSTREAM_INCLUDED
#define NOWTECH_LOG_STD_THREAD_OSTREAM_INCLUDED

#include "Log.h"
#include <map>
#include <set>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>
#include <string>
#include <ostream>
#include <functional>
#include <condition_variable>
#include <boost/lockfree/queue.hpp>

  /*inline static*/ std::atomic<bool>             sKeepRunning = true;
namespace nowtech::log {

template<typename tLogSizeType, TaskIdType tMaxTaskCount, bool tBlocks = false, tLogSizeType tChunkSize = 8u>
class LogStdThreadOstream : public LogInterfaceBase {
public:
  typedef tLogSizeType LogSizeType;
  static constexpr tLogSizeType cChunkSize      = tChunkSize;
  static constexpr TaskIdType   cMaxTaskCount   = tMaxTaskCount;
  static constexpr TaskIdType   cLocalTaskId    = tMaxTaskCount;
  static constexpr TaskIdType   cInvalidTaskId  = std::numeric_limits<TaskIdType>::max();

private:
  inline static constexpr uint32_t cEnqueuePollDelay = 1u;

  struct NameId {
    std::string name;
    TaskIdType  id;

    NameId() : id(0u) {
    }

    NameId(std::string &&aName, TaskIdType const aId) : name(std::move(aName)), id(aId) {
    }
  };

  /// Uses moodycamel::ConcurrentQueue to simulate a FreeRTOS queue.
  inline static class FreeRtosQueue final {
    boost::lockfree::queue<char *> sQueue;
    boost::lockfree::queue<char *> mFreeList;
    std::mutex                     mMutexDataArrived;
    std::mutex                     mMutexDataProcessed;
    std::condition_variable        mConditionVariableDataArrived;
    std::condition_variable        mConditionVariableDataProcessed;
    std::atomic<bool>              mDataProcessed;

    char        *mBuffer;
  
  public:
    /// First implementation, we assume we have plenty of memory.
    FreeRtosQueue(size_t const aBlockCount) noexcept
      : sQueue(aBlockCount)
      , mFreeList(aBlockCount)
      , mBuffer(new char[aBlockCount * cChunkSize]) {
      char *ptr = mBuffer;
      for(size_t i = 0u; i < aBlockCount; ++i) {
        mFreeList.bounded_push(ptr);
        ptr += cChunkSize;
      }
    }

    ~FreeRtosQueue() noexcept {
      delete[] mBuffer;
    }

    bool isEmpty() const noexcept {
      return sQueue.empty();
    }

    void send(char const * const aChunkStart) noexcept {
      bool success;
      do {
        char *payload;
        success = mFreeList.pop(payload);
        if(success) {
          std::copy(aChunkStart, aChunkStart + cChunkSize, payload);
          {
            std::unique_lock<std::mutex> lock(mMutexDataArrived);
            sQueue.bounded_push(payload); // this should always succeed here
          }
          mConditionVariableDataArrived.notify_one();
        }
        else if(tBlocks) {
          std::unique_lock<std::mutex> lock(mMutexDataProcessed);
          mConditionVariableDataProcessed.wait(lock, [this](){
            return mDataProcessed.load();
          });
          mDataProcessed = false;
        }
      } while(tBlocks && !success);
    }

    bool receive(char * const aChunkStart, uint32_t const aPauseLength) noexcept {
      bool result;
      std::unique_lock<std::mutex> lock(mMutexDataArrived);
      // Safe to call empty because there will be only one consumer.
      if(!mConditionVariableDataArrived.wait_for(lock, std::chrono::milliseconds(aPauseLength), [this](){
        return !sQueue.empty();
      })) {
        result = false;
      }
      else {
        char *payload;
        result = sQueue.pop(payload);
        if(result) {
          std::copy(payload, payload + cChunkSize, aChunkStart);
          mFreeList.bounded_push(payload); // this should always succeed here
          {
            std::unique_lock<std::mutex> lock(mMutexDataProcessed);
            mDataProcessed = true;
          }
          mConditionVariableDataProcessed.notify_one();
        }
        else { // nothing to do
        }
      }
      return result;
    }
  } *sQueue;

  /// Used to force transmission of partially filled buffer in a defined
  /// period of time.
  inline static class FreeRtosTimer final {
    uint32_t                     mTimeout;
    std::function<void()>        mLambda;
    std::mutex                   mMutex;
    std::condition_variable      mConditionVariable;
    std::thread                  mTask;
    std::atomic<bool>            mKeepRunning = true;
    std::atomic<bool>            mAlarmed = false;
    std::atomic<bool>            mCondition = false;

  public:
    FreeRtosTimer(uint32_t const aTimeout, std::function<void()> aLambda)
    : mTimeout(aTimeout)
    , mLambda(aLambda) 
    , mTask(&LogStdThreadOstream::FreeRtosTimer::run, this) {
    }

    ~FreeRtosTimer() noexcept {
      {
        std::unique_lock<std::mutex> lock(mMutex);
        mKeepRunning = false;
      }
      mConditionVariable.notify_one();
      mTask.join();
    }
  
    void run() noexcept {
      while(mKeepRunning) {
        {
          std::unique_lock<std::mutex> lock(mMutex);
          mConditionVariable.wait(lock, [this](){
           return !mKeepRunning.load() || mAlarmed.load();
          });
        }
        if(mKeepRunning) {
          std::this_thread::sleep_for(std::chrono::milliseconds(mTimeout));
          mLambda();
          mAlarmed = false;
        }
        else { // nothing to do
        }
      }
    }

    void start() noexcept {
      {
        std::unique_lock<std::mutex> lock(mMutex);
        mAlarmed = true;
      }
      mConditionVariable.notify_one();
    }
  } *sRefreshTimer;

  inline static uint32_t                           sPauseLength;
  inline static std::ostream*                      sOutput;
  inline static std::thread*                       sTransmitterTask;
  inline static std::map<std::thread::id, NameId>* sTaskNamesIds;
  inline static std::set<TaskIdType>*              sFreeTaskIds;

  inline static std::mutex                    sFinishedTxMutex;
  inline static std::condition_variable       sFinishedTxConditionVariable;
  inline static std::atomic<bool>             sFinishedTx = true;
  inline static std::mutex                    sDataArrivedMutex;
  inline static std::condition_variable       sDataArrivedConditionVariable;
  inline static std::mutex                    sRegistrationMutex;

  LogStdThreadOstream() = delete;

  // The below methods are NOT PART of the API.
public:
  // Must be called before Log::init()
  static void init(std::ostream &aOutput) noexcept {
    sOutput = &aOutput;
  }

  // Only Log::init may call this
  static void init(LogConfig const &aConfig, std::function<void()> aTransmitterTaskFunction, std::function<void()> aRefreshNeededFunction) {
    sTaskNamesIds = new std::map<std::thread::id, NameId>();
    sFreeTaskIds = new std::set<TaskIdType>();
    for(TaskIdType i = 0u; i < tMaxTaskCount; ++i) {
      sFreeTaskIds->insert(i);
    }
    sPauseLength = aConfig.pauseLength;
    sQueue = new FreeRtosQueue(aConfig.queueLength);
    sRefreshTimer = new FreeRtosTimer(aConfig.refreshPeriod, aRefreshNeededFunction);
    sTransmitterTask = new std::thread(aTransmitterTaskFunction);
  }

  static void finishedTransmitterTask() noexcept { // nothing to do
  }

  static void done() {
    {
      std::unique_lock<std::mutex> lock(sDataArrivedMutex);
      sKeepRunning = false;
    }
    sDataArrivedConditionVariable.notify_one();
    sTransmitterTask->join();
    delete sTransmitterTask;
    delete sQueue;
    delete sRefreshTimer;
    delete sTaskNamesIds;
    delete sFreeTaskIds;
    sOutput->flush();
  }

  static TaskIdType registerCurrentTask(char const * const aTaskName) noexcept {
    std::lock_guard<std::mutex> lock(sRegistrationMutex);
    TaskIdType nextTaskId;
    auto id = std::this_thread::get_id();
    if(sFreeTaskIds->size() > 0u && sTaskNamesIds->find(id) == sTaskNamesIds->end()) {
      auto first = sFreeTaskIds->begin();
      nextTaskId = *first;
      sFreeTaskIds->erase(first);
      NameId item { std::string(aTaskName == nullptr ? cAnonymousTaskName : aTaskName), nextTaskId };
      sTaskNamesIds->insert(std::pair<std::thread::id, NameId>(id, item));
    }
    else {
      nextTaskId = cInvalidTaskId;
    }
    return nextTaskId;
  }

  static TaskIdType unregisterCurrentTask() noexcept {
    std::lock_guard<std::mutex> lock(sRegistrationMutex);
    TaskIdType foundTaskId;
    auto id = std::this_thread::get_id();
    auto found = sTaskNamesIds->find(id);
    if(found != sTaskNamesIds->end()) {
      sFreeTaskIds->insert(found->second.id);
      sTaskNamesIds->erase(found);
    }
    else {
      foundTaskId = cInvalidTaskId;
    }
    return foundTaskId;
  }

  static char const * getCurrentTaskName() noexcept {
    char const *result;
    if(isInterrupt()) {
      result = cIsrTaskName;
    }
    else {
      auto found = sTaskNamesIds->find(std::this_thread::get_id());
      if(found != sTaskNamesIds->end()) {
        result = found->second.name.c_str();
      }
      else {
        result = cUnknownTaskName;
      }
    }
    return result;
  }

  static TaskIdType getCurrentTaskId(TaskIdType const aTaskId) noexcept {
    TaskIdType result = aTaskId;
    if(aTaskId == cLocalTaskId && !isInterrupt()) {
      auto found = sTaskNamesIds->find(std::this_thread::get_id());
      if(found != sTaskNamesIds->end()) {
        result = found->second.id;
      }
      else {
        result = cInvalidTaskId;
      }
    }
    else { // nothing to do
    }
    return result;
  }

  static uint32_t getLogTime() noexcept {
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
  }

  static bool isInterrupt() noexcept {
    return false;
  }

  static void waitForData() noexcept {
    std::unique_lock<std::mutex> lock(sDataArrivedMutex);
    sDataArrivedConditionVariable.wait(lock, [](){ 
      return !sQueue->isEmpty() || !sKeepRunning.load();
    });
  }

  static void waitWhileTransmitInProgress() noexcept {
    std::unique_lock<std::mutex> lock(sFinishedTxMutex);
    sFinishedTxConditionVariable.wait(lock, [](){ 
      return sFinishedTx.load();
    });
  }

  static void push(char const * const aChunkStart) noexcept {
    {
      std::lock_guard<std::mutex> lock(sDataArrivedMutex);
      sQueue->send(aChunkStart);
    }
    sDataArrivedConditionVariable.notify_one();
  }

  static bool fetch(char * const aChunkStart) noexcept {
    return sQueue->receive(aChunkStart, sPauseLength);
  }

  static void pause() noexcept {
    std::this_thread::sleep_for(std::chrono::milliseconds(sPauseLength));
  }

  static bool isTransmitDone() noexcept {
    return sFinishedTx;
  }

  static void transmit(const char * const aBuffer, LogSizeType const aLength) noexcept {
    sFinishedTx = false;
    sOutput->write(aBuffer, aLength);
    {
      std::lock_guard<std::mutex> lock(sFinishedTxMutex);
      sFinishedTx = true;
    }
    sFinishedTxConditionVariable.notify_one();
  }

  static void startRefreshTimer() noexcept {
    sRefreshTimer->start();
  }
};

} //namespace nowtech

#endif // NOWTECH_LOG_FREERTOS_STMHAL_INCLUDED