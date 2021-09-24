//
// Created by lantiati on 2021. 09. 15.
//

#ifndef NOWTECH_LOG_APP_INTERFACE_AZURERTOS_MINIMAL
#define NOWTECH_LOG_APP_INTERFACE_AZURERTOS_MINIMAL

#include "Log.h"
#include <array>
#include "main.h"
#include "tx_api.h"
#include "OnlyAllocate.h"

extern "C" void logTransmitterTask(ULONG arg);

class Interface final {
public:
  static void badAlloc() {
    Error_Handler();
  }
};

typedef nowtech::memory::OnlyAllocate<Interface> tOnlyAllocator;

namespace nowtech::log {


template<TaskId tMaxTaskCount, bool tLogFromIsr, size_t tTaskShutdownPollPeriod, uint32_t tThreadStackSize>
class AppInterfaceAzureRtosMinimal final {
public:
  using LogTime = uint32_t;
  static constexpr TaskId csMaxTaskCount      = tMaxTaskCount; // Exported just to let the Log de checks.
  static constexpr TaskId csInvalidTaskId     = std::numeric_limits<TaskId>::max();
  static constexpr TaskId csIsrTaskId         = std::numeric_limits<TaskId>::min();
  static constexpr TaskId csFirstNormalTaskId = csIsrTaskId + 1u;
  static constexpr bool   csConstantTaskNames = true;
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
      while(true) {
      }
    }
  };

private:
  inline static constexpr char csUnknownTaskName[]     = "UNKNOWN";
  inline static constexpr char csIsrTaskName[]         = "ISR";
  inline static CHAR csTransmitterTaskName[]           = "logTx";
  inline static CHAR cLogMutexMane[]                   = "LogRegistrationMutex";
  inline static constexpr ULONG csRegistrationMutexTimeout = TX_WAIT_FOREVER ;

  inline static std::array<TX_THREAD*, csMaxTaskCount> sTaskHandles;
  inline static TaskId sPreviousTaskId = csFirstNormalTaskId - 1u;
  inline static TX_MUTEX sRegistrationMutex;
  inline static TX_THREAD sTransmitterTask;
  //TODO pass the therad stack in argument


  AppInterfaceAzureRtosMinimal() = delete;
public:
  static void init() {
    if(tx_mutex_create(&sRegistrationMutex,  cLogMutexMane , TX_INHERIT) != TX_SUCCESS)
    {
      Error_Handler();
    }
  }

  static void init(void (*aFunction)(), uint32_t const aStackDepth, uint32_t const aPriority) {
    init();
    static constexpr ULONG cEntryInput = 0U;
    void *sLoggerThreadStack = tOnlyAllocator::_newArray<uint8_t>(tThreadStackSize);
    UINT status = tx_thread_create(
        &sTransmitterTask,
        csTransmitterTaskName,
        &logTransmitterTask,
        cEntryInput,
        sLoggerThreadStack,
        aStackDepth,
        aPriority,
        aPriority,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if(status != TX_SUCCESS){
      Error_Handler();
    }
  }

  static void done() { // We assume it runs forever.
  }

  /// We assume it won't get called during registering.
  static TaskId getCurrentTaskId() noexcept {
    TaskId result;
    if(isInterrupt()) {
      if constexpr (tLogFromIsr) {
        result = csInvalidTaskId;
      }
      else {
        result = csIsrTaskId;
      }
    }
    else {
      TX_THREAD* taskHandle = tx_thread_identify();
      auto end = sTaskHandles.begin() + sPreviousTaskId;
      auto found = std::find(sTaskHandles.begin(), end, taskHandle);
      if(found != end) {
        result = found - sTaskHandles.begin() + csFirstNormalTaskId - csIsrTaskId;
      }
      else {
        result = csInvalidTaskId;
      }
    }
    return result;
  }

  static TaskId registerCurrentTask(char const * const) {
    tx_mutex_get(&sRegistrationMutex, csRegistrationMutexTimeout);
    TaskId result;
    if(sPreviousTaskId < csMaxTaskCount){
      TX_THREAD* taskHandle = tx_thread_identify();
      auto end = sTaskHandles.begin() + sPreviousTaskId;
      auto found = std::find(sTaskHandles.begin(), end, taskHandle);
      if(found != end) {
        result = found - sTaskHandles.begin() + csFirstNormalTaskId - csIsrTaskId;
      }
      else {
        result = ++sPreviousTaskId;
        *end = taskHandle;
      }
    }
    else {
      result = csInvalidTaskId;
    }
    tx_mutex_put(&sRegistrationMutex);
    return result;
  }

  static TaskId unregisterCurrentTask() { // We assume everything runs forever.
    return csInvalidTaskId;
  }

  // Application can trick supplied task IDs to make this function return a dangling pointer if concurrent task unregistration
  // occurs. Normal usage should be however safe.
  static char const * getTaskName(TaskId const aTaskId) noexcept {
    char const * result;
    if(isInterrupt()) {
      result = csIsrTaskName;
    }
    else {
      if(aTaskId <= sPreviousTaskId) {
        UINT state;
        CHAR *name;
        ULONG runCount;
        UINT priority;
        UINT preemptionThreshold;
        ULONG timeSlice;
        TX_THREAD *nextThread;
        TX_THREAD *suspendedThread;
        UINT status;

        // Retrieve information about thread
        status = tx_thread_info_get(sTaskHandles[aTaskId - csFirstNormalTaskId + csIsrTaskId], &name,
                                    &state, &runCount,
                                    &priority, &preemptionThreshold,
                                    &timeSlice, &nextThread,&suspendedThread);
        if(status != TX_SUCCESS) {
          Error_Handler();
        }
        result = name;
      }
      else {
        result = csUnknownTaskName;
      }
    }
    return result;
  }

  static LogTime getLogTime() noexcept {
    return tx_time_get(); //Log time in [ms]
  }

  static void finish() noexcept { // We assume everything runs forever.
  }

  static void waitForFinished() noexcept { // We assume everything runs forever.
  }

  static void sleepWhileWaitingForTaskShutdown() noexcept {
    tx_thread_sleep(tTaskShutdownPollPeriod / TX_TIMER_TICKS_PER_SECOND );
  }


  static void lock() noexcept { // Now don't care.
  }

  static void unlock() noexcept { // Now don't care.
  }

  static void error(Exception const aError) {
	  while(true) {
	  }
  }

  static void fatalError(Exception const) {
	  while(true) {
	  }
  }

  static void sleep(const ULONG aTimerTicks){
    tx_thread_sleep(aTimerTicks);
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

private:
  static bool isInterrupt() noexcept {   // Well, this is STM32-specific, but can easily be changed to other MCUs as well.
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
  }

};

}

inline void *operator new(size_t size) {
 void* retval = tOnlyAllocator::_newArray<uint8_t>(size) ;
 return retval;
}

inline void operator delete(void *aPointer) noexcept {
    Error_Handler();
}

inline void operator delete(void *aPointer, size_t) noexcept {
  Error_Handler();
}

inline void *operator new[](size_t size) {
  return tOnlyAllocator::_newArray<uint8_t>(size);
}

inline void operator delete[](void *aPointer) noexcept {
  Error_Handler();
}

inline void operator delete[](void *aPointer, size_t) noexcept {
  Error_Handler();
}

#endif
