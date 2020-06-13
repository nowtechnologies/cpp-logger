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

#include "ArrayMap.h"
#include "Log.h"
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

namespace nowtech::log {

template<typename tLogSizeType, bool tBlocks = false, tLogSizeType tChunkSize = 8u>
class LogStdThreadOstream : public LogInterfaceBase {
public:
  typedef tLogSizeType LogSizeType;
  static constexpr tLogSizeType cChunkSize = tChunkSize;

private:
  static constexpr uint32_t cInvalidGivenTaskId = 0u;
  inline static constexpr uint32_t cEnqueuePollDelay = 1u;

  struct NameId {
    std::string name;
    uint32_t    id;

    NameId() : id(0u) {
    }

    NameId(std::string &&aName, uint32_t const aId) : name(std::move(aName)), id(aId) {
    }
  };

  /// Uses moodycamel::ConcurrentQueue to simulate a FreeRTOS queue.
  inline static class FreeRtosQueue final {
    boost::lockfree::queue<char *> sQueue;
    boost::lockfree::queue<char *> mFreeList;
    std::mutex                     mMutex;
    std::unique_lock<std::mutex>   mLock;
    std::condition_variable        mConditionVariable;

    char        *mBuffer;
  
  public:
    /// First implementation, we assume we have plenty of memory.
    FreeRtosQueue(size_t const aBlockCount) noexcept
      : sQueue(aBlockCount)
      , mFreeList(aBlockCount)
      , mLock(mMutex)
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

    void send(char const * const aChunkStart) noexcept {
      bool success;
      do {
        char *payload;
        success = mFreeList.pop(payload);
        if(success) {
          std::copy(aChunkStart, aChunkStart + cChunkSize, payload);
          sQueue.bounded_push(payload); // this should always succeed here
          mConditionVariable.notify_one();
        }
        else {
          std::this_thread::sleep_for(std::chrono::milliseconds(cEnqueuePollDelay)); // TODO eliminate
        }
      } while(tBlocks && !success);
    }

    bool receive(char * const aChunkStart, uint32_t const aPauseLength) noexcept {
      bool result;
      // Safe to call empty because there will be only one consumer.
      if(sQueue.empty() && mConditionVariable.wait_for(mLock, std::chrono::milliseconds(aPauseLength)) == std::cv_status::timeout) {
        result = false;
      }
      else {
        char *payload;
        result = sQueue.pop(payload);
        if(result) {
          std::copy(payload, payload + cChunkSize, aChunkStart);
          mFreeList.bounded_push(payload); // this should always succeed here
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
    std::unique_lock<std::mutex> mLock;
    std::condition_variable      mConditionVariable;
    std::thread                  mThread;
    std::atomic<bool>            mKeepRunning = true;
    std::atomic<bool>            mAlarmed = false;

  public:
    FreeRtosTimer(uint32_t const aTimeout, std::function<void()> aLambda)
    : mTimeout(aTimeout)
    , mLambda(aLambda) 
    , mLock(mMutex)
    , mThread(&LogStdThreadOstream::FreeRtosTimer::run, this) {
    }

    ~FreeRtosTimer() noexcept {
      mKeepRunning = false;
      mConditionVariable.notify_one();
      mThread.join();
    }
  
    void run() noexcept {
      while(mKeepRunning) {
        if(mConditionVariable.wait_for(mLock, std::chrono::milliseconds(mTimeout)) == std::cv_status::timeout && mAlarmed) {
          mLambda();
          mAlarmed = false;
        }
        else {
        }
      }
    }

    void start() noexcept {
      mAlarmed = true;
      mConditionVariable.notify_one();
    }
  } *sRefreshTimer;

  inline static uint32_t sPauseLength;
  inline static std::ostream* sOutput;
  inline static std::thread* sTransmitterThread;
  inline static std::map<std::thread::id, NameId>* sTaskNamesIds;
  inline static uint32_t sNextGivenTaskId = cInvalidGivenTaskId + 1u; // TODO consider

  /// True if the partially filled buffer should be sent. This is
  /// defined here because OS-specific functionality is here.
  inline static std::atomic<bool>*            sRefreshNeeded;

  /// We use std::recursive_mutex here (banned by HIC++4), because the OsInterface API
  /// was designed for FreeRTOS, and we currently have no resource to redesign it.
  inline static std::recursive_mutex          sApiMutex;
  inline static std::mutex                    sMutex;
  inline static std::condition_variable       sConditionVariable;
  inline static std::atomic<bool>             sCondition = false;

  LogStdThreadOstream() = delete;

  // The below methods are NOT PART of the API.
public:
  // Must be called before Log::init()
  static void init(std::ostream &aOutput) noexcept {
    sOutput = &aOutput;
  }

  // Only Log::init may call this
  static void init(LogConfig const &aConfig, std::function<void()> aTransmitterThreadFunction) {
    sTaskNamesIds = new std::map<std::thread::id, NameId>();
    sPauseLength = aConfig.pauseLength;
    sQueue = new FreeRtosQueue(aConfig.queueLength);
    sRefreshTimer = new FreeRtosTimer(aConfig.refreshPeriod, []{refreshNeeded();});
    sTransmitterThread = new std::thread(aTransmitterThreadFunction);
  }

  static void finishedTransmitterThread() noexcept {
    std::lock_guard<std::mutex> lock(sMutex);
    sCondition = true;
    sConditionVariable.notify_one();
  }

  static void done() {
    std::unique_lock<std::mutex> lock(sMutex);
    sConditionVariable.wait(lock, [](){ return sCondition.load(); });
    sTransmitterThread->join();
    delete sTransmitterThread;
    delete sQueue;
    delete sRefreshTimer;
    delete sTaskNamesIds;
    sOutput->flush();
  }

  static void registerThreadName(char const * const aTaskName) noexcept {
    NameId item { std::string(aTaskName), sNextGivenTaskId };
    ++sNextGivenTaskId;
    sTaskNamesIds->insert(std::pair<std::thread::id, NameId>(std::this_thread::get_id(), item));
  }

  static char const * getCurrentThreadName() noexcept {
    char const *result;
    auto found = sTaskNamesIds->find(std::this_thread::get_id());
    if(found != sTaskNamesIds->end()) {
      result = found->second.name.c_str();
    }
    else {
      result = cUnknownApplicationName;
    }
    return result;
  }

  static uint32_t getCurrentThreadId() noexcept {
    uint32_t result;
    auto found = sTaskNamesIds->find(std::this_thread::get_id());
    if(found != sTaskNamesIds->end()) {
      result = found->second.id;
    }
    else {
      result = cInvalidGivenTaskId;
    }
    return result;
  }

  static uint32_t getLogTime() noexcept {
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
  }

  static bool isInterrupt() noexcept {
    return false;
  }

  static void push(char const * const aChunkStart) noexcept {
    sQueue->send(aChunkStart);
  }

  static bool fetch(char * const aChunkStart) noexcept {
    return sQueue->receive(aChunkStart, sPauseLength);
  }

  static void pause() noexcept {
    std::this_thread::sleep_for(std::chrono::milliseconds(sPauseLength));
  }

  static void transmit(const char * const aBuffer, LogSizeType const aLength, std::atomic<bool> *aProgressFlag) noexcept {
    sOutput->write(aBuffer, aLength);
    aProgressFlag->store(false);
  }

  static void startRefreshTimer(std::atomic<bool> *aRefreshFlag) noexcept {
    sRefreshNeeded = aRefreshFlag;
    sRefreshTimer->start();
  }

  static void refreshNeeded() noexcept {
    sRefreshNeeded->store(true);
  }

  static void lock() noexcept {
    sApiMutex.lock();
  }

  static void unlock() noexcept {
    sApiMutex.unlock();
  }
};

} //namespace nowtech

#endif // NOWTECH_LOG_FREERTOS_STMHAL_INCLUDED












