#ifndef LOG_APP_INTERFACE_STD
#define LOG_APP_INTERFACE_STD

#include "Log.h"
#include <ios>
#include <map>
#include <set>
#include <mutex>
#include <chrono>
#include <thread>
#include <functional>

namespace nowtech::log {

template<LogTopic tMaxTaskCount, bool tLogFromIsr>
class AppInterfaceStd final {
public:
  using LogTime = uint32_t;
  static constexpr TaskId csMaxTaskCount      = tMaxTaskCount; // Exported just to let the Log de checks.
  static constexpr TaskId csInvalidTaskId     = std::numeric_limits<TaskId>::max();
  static constexpr TaskId csIsrTaskId         = std::numeric_limits<TaskId>::min();
  static constexpr TaskId csFirstNormalTaskId = csIsrTaskId + 1u;

private:
  inline static constexpr char csErrorMessages[static_cast<size_t>(Exception::cCount)][40] = {
    "cOutOfTaskIdsOrDoubleRegistration", "cOutOfTopics", "cSenderError"
  };
  inline static constexpr char csError[]           = "Error: ";
  inline static constexpr char csFatalError[]      = "Fatal: ";
  inline static constexpr char csUnknownTaskName[] = "UNKNOWN";
  inline static constexpr char csIsrTaskName[]     = "ISR";

  struct TaskNameId {
    std::string mName;
    uint32_t    mId;

    TaskNameId() : mId(0u) {
    }

    TaskNameId(std::string &&aName, uint32_t const aId) : mName(std::move(aName)), mId(aId) {
    }
  };

  inline static std::map<std::thread::id, TaskNameId> sThreadId2taskNamesIds;
  inline static std::map<TaskId, std::thread::id> sTaskId2threads;
  inline static std::set<TaskId> sFreeTaskIds;
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
    }
    else { // nothing to do
    }
    sThreadId2taskNamesIds.clear();
    sTaskId2threads.clear();
    sFreeTaskIds.clear();
  }

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
      sTaskId2threads.erase(found->second.mId);
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
    auto foundThreadId = sTaskId2threads.find(aTaskId);
    char const * result;
    if(foundThreadId != sTaskId2threads.end()) {
      auto foundTaskNameId = sThreadId2taskNamesIds.find(foundThreadId->second);
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