#ifndef LOG_QUEUE_STD_BOOST
#define LOG_QUEUE_STD_BOOST

#include <cstddef>
#include <mutex>
#include <condition_variable>
#include <boost/lockfree/queue.hpp>

namespace nowtech::log {

template<typename tMessage, size_t tQueueSize>
class QueueStdBoost final {
public:
  using tMessage_ = tMessage;

private:
  class FreeRtosQueue final {
    boost::lockfree::queue<tMessage> mQueue;
    std::atomic<bool>              mNotified;
    std::mutex                     mMutex;
    std::unique_lock<std::mutex>   mLock;
    std::condition_variable        mConditionVariable;

  public:
    /// First implementation, we assume we have plenty of memory.
    FreeRtosQueue() noexcept
      : mQueue(tQueueSize)
      , mLock(mMutex) {
      mNotified = false;
    }

    ~FreeRtosQueue() noexcept = default;

    void push(tMessage const &aMessage) noexcept {
      bool success = mQueue.bounded_push(aMessage);
      if(success) {
        mNotified = true;
        mConditionVariable.notify_one();
      }
      else { // nothing to do
      }
    }

    bool pop(tMessage &aMessage, uint32_t const mPauseLength) noexcept {
      bool result;
      // Safe to call empty because there will be only one consumer.
      if(mQueue.empty() && mConditionVariable.wait_for(mLock, std::chrono::milliseconds(mPauseLength), [this]{return mNotified;}) == std::cv_status::timeout) {
        result = false;
      }
      else {
        mNotified = false;
        result = mQueue.pop(aMessage);
      }
      return result;
    }
  };

  inline static FreeRtosQueue sQueue;

  QueueStdBoost() = delete;

public:
  static void init() { // nothing to do
  }

  static void done() {  // nothing to do
  }

  static void push(tMessage const &aMessage) noexcept {
    sQueue.push(aMessage);
  }
};

}

#endif