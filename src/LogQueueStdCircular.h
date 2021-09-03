#ifndef LOG_QUEUE_STD_BOOST
#define LOG_QUEUE_STD_BOOST

#include <array>
#include <cstddef>
#include <mutex>
#include <condition_variable>

namespace nowtech::log {

template<typename tMessage, typename tAppInterface, size_t tQueueSize>
class QueueStdCircular final {
public:
  using tMessage_ = tMessage;
  using tAppInterface_ = tAppInterface;
  using LogTime = typename tAppInterface::LogTime;

  static constexpr size_t csQueueSize = tQueueSize;

private:
  class FreeRtosQueue final {
    std::array<tMessage, tQueueSize> mQueue;
    size_t                           mNextWrite;
    size_t                           mNextRead;
    std::atomic<size_t>              mOccupied;
    std::atomic<bool>                mNotified;
    std::mutex                       mMutexPush;
    std::mutex                       mMutexCv;
    std::unique_lock<std::mutex>     mLockCv;
    std::condition_variable          mConditionVariable;

  public:
    /// First implementation, we assume we have plenty of memory.
    FreeRtosQueue() noexcept
      : mNextWrite(0u)
      , mNextRead(0u)
      , mOccupied(0u)
      , mLockCv(mMutexCv) {
      mNotified = false;
    }

    ~FreeRtosQueue() noexcept = default;

    bool empty() const noexcept {
      return mOccupied == 0u;
    }

    void push(tMessage const &aMessage) noexcept {
      std::lock_guard<std::mutex> lock(mMutexPush);
      if(mOccupied < tQueueSize) {
        mQueue[mNextWrite] = aMessage;
        mNextWrite = (mNextWrite + 1u) % tQueueSize;
        ++mOccupied;        
        mNotified = true;         // Having no lock_guard here makes tremendous speedup.
        mConditionVariable.notify_one();
      }
      else { // nothing to do
      }
    }

    bool pop(tMessage &aMessage, LogTime const mPauseLength) noexcept {
      bool result;
      // Safe to call empty because there will be only one consumer.
      if(mOccupied == 0u &&
        (!mConditionVariable.wait_for(mLockCv, std::chrono::milliseconds(mPauseLength), [this]{return mNotified == true;})
        && !mNotified )) {  // This check makes the lock_guard on notifying unnecessary,
        // because I don't care if I get in right during waiting or just at the beginning of next check.
        result = false;
      }
      else {
        mNotified = false;
        aMessage = mQueue[mNextRead];
        mNextRead = (mNextRead + 1u) % tQueueSize;
        --mOccupied;
        result = true;
      }
      return result;
    }
  };

  inline static FreeRtosQueue sQueue;

  QueueStdCircular() = delete;

public:
  static void init() { // nothing to do
  }

  static void done() {  // nothing to do
  }

  static bool empty() noexcept {
    return sQueue.empty();
  }

  static void push(tMessage const &aMessage) noexcept {
    sQueue.push(aMessage);
  }

  static bool pop(tMessage &aMessage, LogTime const aPauseLength) noexcept {
    return sQueue.pop(aMessage, aPauseLength);
  }
};

}

#endif
