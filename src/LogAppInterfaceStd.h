#ifndef NOWTECH_LOG_APP_INTERFACE_STD
#define NOWTECH_LOG_APP_INTERFACE_STD

#include "Log.h"
#include <ios>
#include <array>
#include <mutex>
#include <chrono>
#include <thread>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <condition_variable>

namespace nowtech::log {

template<LogTopic tMaxTaskCount, bool tLogFromIsr, size_t tTaskShutdownSleepPeriod>
class AppInterfaceStd final {
public:
  using LogTime = uint32_t;
  static constexpr TaskId csMaxTaskCount      = tMaxTaskCount; // Exported just to let the Log de checks.
  static constexpr TaskId csInvalidTaskId     = std::numeric_limits<TaskId>::max();
  static constexpr TaskId csIsrTaskId         = std::numeric_limits<TaskId>::min();
  static constexpr TaskId csFirstNormalTaskId = csIsrTaskId + 1u;

  class Occupier final {
  public:
    Occupier() = default;

    void* occupy(size_t const aSize) noexcept {
      return new std::byte[aSize];
    }

    void release(void* const aPointer) noexcept {
      delete[](static_cast<std::byte*>(aPointer));
    }
    
    void badAlloc() {
      throw std::bad_alloc();
    }
  };

private:
  class Semaphore final {
  private:
    std::atomic<bool>              mNotified = false;
    std::mutex                     mMutex;
    std::unique_lock<std::mutex>   mLock;
    std::condition_variable        mConditionVariable;

  public:
    Semaphore() noexcept : mLock(mMutex) {
    }

    void wait() noexcept {
      mConditionVariable.wait(mLock, [this] { return mNotified == true; });
    }

    void notify() noexcept {
      mNotified = true;
      mConditionVariable.notify_one();
    }
  };

  inline static Semaphore sSemaphore;

  inline static constexpr char csErrorMessages[static_cast<size_t>(Exception::cCount)][40] = {
    "cOutOfTaskIdsOrDoubleRegistration", "cOutOfTopics", "cSenderError"
  };
  inline static constexpr char   csError[]            = "Error: ";
  inline static constexpr char   csFatalError[]       = "Fatal: ";
  inline static constexpr char   csUnknownTaskName[]  = "UNKNOWN";
  inline static constexpr char   csIsrTaskName[]      = "ISR";
  inline static constexpr size_t csTaskId2threadsSize = csMaxTaskCount + 1u;
  inline static const std::thread::id csNoThreadId;

  struct TaskNameId {
    std::string mName;
    uint32_t    mId;

    TaskNameId() : mId(0u) {
    }

    TaskNameId(std::string &&aName, uint32_t const aId) : mName(std::move(aName)), mId(aId) {
    }
  };

  inline static std::unordered_map<std::thread::id, TaskNameId> sThreadId2taskNamesIds;
  inline static std::array<std::thread::id, csTaskId2threadsSize> sTaskId2threads;
  inline static std::unordered_set<TaskId> sFreeTaskIds;
  inline static std::mutex sRegistrationMutex;
  inline static std::thread *sTransmitterThread;
  
  AppInterfaceStd() = delete;

public:
  static void init() {
    for(TaskId id = csFirstNormalTaskId; id <= tMaxTaskCount; ++id) {
      sFreeTaskIds.insert(id);
    }
  }

  static void init(std::function<void(void)> aFunction) {
    init();
    sTransmitterThread = new std::thread(aFunction);
  }

  static void done() {
    if(sTransmitterThread != nullptr) {
      sTransmitterThread->join();
      delete sTransmitterThread;
    }
    else { // nothing to do
    }
    sThreadId2taskNamesIds.clear();
    sFreeTaskIds.clear();
  }

  /// We assume it won't get called during registering.
  static TaskId getCurrentTaskId() noexcept { // TODO in other implementations returns 0 if ISR and it is enabled. If disabled, csInvalidTaskId.
    auto threadId = std::this_thread::get_id();
    TaskId result;
    auto foundTaskNameId = sThreadId2taskNamesIds.find(threadId);
    if(foundTaskNameId != sThreadId2taskNamesIds.end()) {
      result = foundTaskNameId->second.mId;
    }
    else {
      result = csInvalidTaskId;
    }
    return result;
  }

  static TaskId registerCurrentTask(char const * const aTaskName) {
    std::lock_guard<std::mutex> lock (sRegistrationMutex);
    TaskId result;
    if(!sFreeTaskIds.empty()) {
      auto begin = sFreeTaskIds.begin();
      result = *begin;
      auto threadId = std::this_thread::get_id();
      auto [iterator, inserted] = sThreadId2taskNamesIds.try_emplace(threadId, TaskNameId{std::string(aTaskName), result});
      if(inserted) {
        sTaskId2threads[result] = threadId;
        sFreeTaskIds.erase(begin);
      }
      else { // nothing to do
      }
    }
    else {
      result = csInvalidTaskId;
    }
    return result;
  }

  static TaskId unregisterCurrentTask() {
    std::lock_guard<std::mutex> lock (sRegistrationMutex);
    TaskId result;
    auto found = sThreadId2taskNamesIds.find(std::this_thread::get_id());
    if(found != sThreadId2taskNamesIds.end()) {
      result = found->second.mId;
      sTaskId2threads[found->second.mId] = std::thread::id{};
      sThreadId2taskNamesIds.erase(found);
      sFreeTaskIds.insert(result);
    }
    else {
      result = csInvalidTaskId;
    }
    return result;
  }

  // Application can trick supplied task IDs to make this function return a dangling pointer if concurrent task unregistration
  // occurs. Normal usage should be however safe.
  static char const * getTaskName(TaskId const aTaskId) noexcept { // TODO for other implementations: if in ISR, returns "ISR"
    auto found = sTaskId2threads[aTaskId];
    char const * result;
    if(found != csNoThreadId) {
      auto foundTaskNameId = sThreadId2taskNamesIds.find(found);
      if(foundTaskNameId != sThreadId2taskNamesIds.end()) {
        result = foundTaskNameId->second.mName.c_str();
      }
      else {  // should not occur
        result = csUnknownTaskName;  
      }
    }
    else {
      result = csUnknownTaskName;
    }
    return result;
  }

  static LogTime getLogTime() noexcept {
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
  }

  static void finish() noexcept {
    sSemaphore.notify();
  }

  static void waitForFinished() noexcept {
    sSemaphore.wait();
  }

  static void sleepWhileWaitingForTaskShutdown() noexcept {
    std::this_thread::sleep_for(std::chrono::milliseconds(tTaskShutdownSleepPeriod));
  }

  static void lock() noexcept { // Now don't care.
  }

  static void unlock() noexcept { // Now don't care.
  }

  static void error(Exception const aError) {
    throw std::ios_base::failure(csErrorMessages[static_cast<size_t>(aError)]);
  }

  static void fatalError(Exception const aError) {
    throw std::ios_base::failure(csErrorMessages[static_cast<size_t>(aError)]);
  }

  template<typename tClass, typename ...tParameters>
  static tClass* _new(tParameters... aParameters) {
    return ::new tClass(aParameters...);
  }

  template<typename tClass>
  static tClass* _newArray(uint32_t const aCount) {
    return ::new tClass[aCount];
  }

  template<typename tClass>
  static void _delete(tClass* aPointer) {
    ::delete aPointer;
  }

  template<typename tClass>
  static void _deleteArray(tClass* aPointer) {
    ::delete[] aPointer;
  }
};

}

#endif
