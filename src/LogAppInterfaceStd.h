#ifndef LOG_APP_INTERFACE_STD
#define LOG_APP_INTERFACE_STD

#include "Log.h"
#include <ios>

namespace nowtech::log {

template<LogTopic tMaxTaskCount, bool tLogFromIsr>
class AppInterfaceStd final {
public:
  using LogTime = uint32_t;
  static constexpr TaskId csMaxTaskCount = tMaxTaskCount; // Exported just to let the Log de checks.
  static constexpr TaskId csInvalidTaskId = std::numeric_limits<TaskId>::max();

private:
  inline static constexpr char csErrorMessages[static_cast<size_t>(Exception::cCount)][40] = {
    "cOutOfTaskIdsOrDoubleRegistration", "cOutOfTopics", "cSenderError"
  };
  inline static constexpr char csError[] = "Error: ";
  inline static constexpr char csFatalError[] = "Error: ";

  AppInterfaceStd() = delete;

public:
  static void init() { // nothing to do
  }

  static TaskId getCurrentTaskId() noexcept {

  }

  static char const * getTaskName(TaskId const aTaskId) noexcept {

  }

  static LogTime getLogTime() noexcept {
    
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