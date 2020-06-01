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

    size_t const mBlockSize;
    char        *mBuffer;
  
  public:
    /// First implementation, we assume we have plenty of memory.
    FreeRtosQueue(size_t const aBlockCount, size_t const aBlockSize) noexcept
      : sQueue(aBlockCount)
      , mFreeList(aBlockCount)
      , mLock(mMutex)
      , mBlockSize(aBlockSize) 
      , mBuffer(new char[aBlockCount * aBlockSize]) {
      char *ptr = mBuffer;
      for(size_t i = 0u; i < aBlockCount; ++i) {
        mFreeList.bounded_push(ptr);
        ptr += aBlockSize;
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
          std::copy(aChunkStart, aChunkStart + mBlockSize, payload);
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
          std::copy(payload, payload + mBlockSize, aChunkStart);
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

  /// Length of a pause in ms during waiting for transmission.
  inline static uint32_t sPauseLength;

  /// The output stream to use.
  inline static std::ostream *sOutput;

  /// The transmitter task.
  inline static std::thread *sTransmitterThread;

  inline static std::map<std::thread::id, NameId> sTaskNamesIds;

  inline static uint32_t sNextGivenTaskId = cInvalidGivenTaskId + 1u; // TODO consider

  /// True if the partially filled buffer should be sent. This is
  /// defined here because OS-specific functionality is here.
  inline static std::atomic<bool> *sRefreshNeeded;

  /// We use std::recursive_mutex here (banned by HIC++4), because the OsInterface API
  /// was designed for FreeRTOS, and we currently have no resource to redesign it.
  inline static std::recursive_mutex         sApiMutex;

  LogStdThreadOstream() = delete;

public:
  // Must be called before Log::init()
  static void init(std::ostream &aOutput) noexcept {
    sOutput = &aOutput;
  }

  // Only Log::init may call this
  static void init(LogConfig const &aConfig, std::function<void()> aTransmitterThreadFunction) {
    sPauseLength = aConfig.pauseLength;
    sQueue = new FreeRtosQueue(aConfig.queueLength, tChunkSize);
    sRefreshTimer = new FreeRtosTimer(aConfig.refreshPeriod, []{refreshNeeded();});
    sTransmitterThread = new std::thread(aTransmitterThreadFunction);
  }

  static void finishedTransmitterThread() noexcept { // nothing to do
  }

  static void done() {
    sTransmitterThread->join();
    delete sTransmitterThread;
    delete sQueue;
    delete sRefreshTimer;
    sOutput->flush();
  }

  /// Registers the given name and an artificial ID in a local map.
  /// This function MUST NOT be called from user code.
  /// void Log::registerCurrentTask(char const * const aTaskName) may call it only.
  /// @param aTaskName Task name to register.
  static void registerThreadName(char const * const aTaskName) noexcept {
    NameId item { std::string(aTaskName), sNextGivenTaskId };
    ++sNextGivenTaskId;
    sTaskNamesIds.insert(std::pair<std::thread::id, NameId>(std::this_thread::get_id(), item));
  }

  /// Returns the task name. This is a dummy and inefficient implementation,
  /// but normally runs only once during registering the current thread.
  /// Note, the returned pointer is valid only as long as this object lives.
  static char const * getThreadName(uint32_t const aHandle) noexcept {
    char const * result = "";
    for(auto const &iterator : sTaskNamesIds) {
      if(iterator.second.id == aHandle) {
        result = iterator.second.name.c_str();
      }
      else { // nothing to do
      }
    }
    return result;
  }
  /// Returns the current task name.
  static char const * getCurrentThreadName() noexcept {
    char const *result;
    auto found = sTaskNamesIds.find(std::this_thread::get_id());
    if(found != sTaskNamesIds.end()) {
      result = found->second.name.c_str();
    }
    else {
      result = cUnknownApplicationName;
    }
    return result;
  }

  /// Returns an artificial thread ID for registered threads, cInvalidGivenTaskId otherwise;
  static uint32_t getCurrentThreadId() noexcept {
    uint32_t result;
    auto found = sTaskNamesIds.find(std::this_thread::get_id());
    if(found != sTaskNamesIds.end()) {
      result = found->second.id;
    }
    else {
      result = cInvalidGivenTaskId;
    }
    return result;
  }

  /// Returns the std::chrono::steady_clock tick count converted into ms and truncated to 32 bits.
  static uint32_t getLogTime() noexcept {
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
  }

  static bool isInterrupt() noexcept {
    return false;
  }

  /// Enqueues the chunks, possibly blocking if the queue is full.
  static void push(char const * const aChunkStart) noexcept {
    sQueue->send(aChunkStart);
  }

  /// Removes the oldest chunk from the queue.
  static bool fetch(char * const aChunkStart) noexcept {
    return sQueue->receive(aChunkStart, sPauseLength);
  }

  /// Pauses execution for the period given in the constructor.
  static void pause() noexcept {
    std::this_thread::sleep_for(std::chrono::milliseconds(sPauseLength));
  }

  /// Transmits the data using the serial descriptor given in the constructor.
  /// @param buffer start of data
  /// @param length length of data
  /// @param aProgressFlag address of flag to be set on transmission end.
  static void transmit(const char * const aBuffer, LogSizeType const aLength, std::atomic<bool> *aProgressFlag) noexcept {
    sOutput->write(aBuffer, aLength);
    aProgressFlag->store(false);
  }

  /// Starts the timer after which a partially filled buffer should be sent.
  static void startRefreshTimer(std::atomic<bool> *aRefreshFlag) noexcept {
    sRefreshNeeded = aRefreshFlag;
    sRefreshTimer->start();
  }

  /// Sets the flag.
  static void refreshNeeded() noexcept {
    sRefreshNeeded->store(true);
  }

  /// Calls az OS-specific lock to acquire a critical section, if implemented
  static void lock() noexcept {
    sApiMutex.lock();
  }

  /// Calls az OS-specific lock to release critical section, if implemented
  static void unlock() noexcept {
    sApiMutex.unlock();
  }
};

} //namespace nowtech

#endif // NOWTECH_LOG_FREERTOS_STMHAL_INCLUDED












