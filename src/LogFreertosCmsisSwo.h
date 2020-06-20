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

#ifndef NOWTECH_LOGFREERTOSCMSISSWO_H_INCLUDED
#define NOWTECH_LOGFREERTOSCMSISSWO_H_INCLUDED

#include "Log.h"
#include "CmsisOsUtils.h"
#include "stm32f2xx_hal.h"
#include "cmsis_os.h"  // we use only an assembly macro, nothing more CMSIS-related
#include "FreeRTOS.h"
#include "projdefs.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "queue.h"
#include "stm32utils.h"
#include <atomic>

extern "C" void logRefreshNeededFreeRtosCmsisSwo(TimerHandle_t);

namespace nowtech {

  /// Class implementing log interface for FreeRTOS and STM HAL as STM32CubeMX
  /// provides them.
  class LogFreeRtosCmsisSwo final : public LogOsInterface {
  private:
    /// Stack length of the transmitter task. It should be large enough to
    /// accomodate Log.transmitStackBufferLength
    uint16_t const mTaskStackLength;

    /// Priority for the transmitter task.
    UBaseType_t const mPriority;

    /// The transmitter task's handle.
    TaskHandle_t mTaskHandle = nullptr;

    /// Used for mutual exclusion for the Log.buffer
    QueueHandle_t mQueue;

    /// Used to force transmission of partially filled buffer in a defined
    /// period of time.
    TimerHandle_t mRefreshTimer;

    /// Used for lock and unlock calls.
    SemaphoreHandle_t         mApiGuard;

    /// True if the partially filled buffer should be sent. This is
    /// defined here because OS-specific functionality is here.
    static std::atomic<bool> *sRefreshNeeded;

  public:
    /// Sets parameters and creates the mutex for locking.
    /// @param aConfig config.
    /// @param aTaskStackLength stack length of the transmitter task
    /// @param aPriority of the tranmitter task
    LogFreeRtosCmsisSwo(LogConfig const & aConfig
      , uint16_t const aTaskStackLength
      , UBaseType_t const aPriority) noexcept
      : LogOsInterface(aConfig)
      , mTaskStackLength(aTaskStackLength)
      , mPriority(aPriority) {
      mQueue = xQueueCreate(aConfig.queueLength, cChunkSize);
      mRefreshTimer = xTimerCreate("LogRefreshTimer", pdMS_TO_TICKS(mRefreshPeriod), pdFALSE, nullptr, logRefreshNeededFreeRtosCmsisSwo);
      mApiGuard = xSemaphoreCreateMutex();
    }

    /// This object is not intended to be deleted, so control should never
    /// get here.
    static ~LogFreeRtosCmsisSwo() {
      vQueueDelete(mQueue);
      xTimerDelete(mRefreshTimer, 0);
      vSemaphoreDelete(mApiGuard);
    }

    /// Returns true if we are in an ISR.
    static bool isInterrupt() noexcept {
      return stm32utils::isInterrupt();
    }

    /// Returns the task name.
    /// Must not be called from ISR.
    static const char * getTaskName(uint32_t const aHandle) noexcept {
      return pcTaskGetName(reinterpret_cast<TaskHandle_t>(aHandle));
    }

    /// Returns the task name.
    /// Must not be called from ISR.
    static const char * getCurrentTaskName() noexcept {
      return pcTaskGetName(nullptr);
    }

    /// Returns the FreeRTOS-specific thread ID.
    /// Must not be called from ISR.
    static uint32_t getCurrentTaskId() noexcept {
      return reinterpret_cast<uint32_t>(xTaskGetCurrentTaskHandle());
    }

    /// Returns the FreeRTOS tick count converted into ms.
    static uint32_t getLogTime() const noexcept {
      return nowtech::OsUtil::getUptimeMillis();
    }

    /// Creates the transmitter thread using the name logtransmitter.
    /// @param log the Log object to operate on.
    /// @param threadFunc the C function which serves as the task body and
    /// which will call Log.transmitterTask.
    static void createTransmitterTask(Log *aLog, void(* aTaskFunc)(void *)) noexcept {
      xTaskCreate(aTaskFunc, "logtransmitter", mTaskStackLength, aLog, mPriority, &mTaskHandle);
    }

    /// Joins the thread.
    static void joinTransmitterTask() noexcept {
      vTaskDelete(mTaskHandle);
    }

    /// Enqueues the chunks, possibly blocking if the queue is full.
    /// In ISR, we do not use pxHigherPriorityTaskWoken because
    /// if it ever causes a problem, the first thing to do will
    /// be to remove logging from ISR.
    /// "If you set the parameter to NULL, the context switch
    /// will happen at the next tick (ticks happen each millisecond
    /// if you are using the default settings), not immediatlely after
    /// the end of the ISR. Depending on your use-case this may
    /// be acceptable or not."
    /// https://stackoverflow.com/questions/28985010/about-pxhigherprioritytaskwoken
    static void push(char const * const aChunkStart, bool const aBlocks) noexcept {
      if(stm32utils::isInterrupt()) {
        BaseType_t higherPriorityTaskWoken;
        xQueueSendFromISR(mQueue, aChunkStart, &higherPriorityTaskWoken);
        if(higherPriorityTaskWoken == pdTRUE) {
          //TODO consider if needed. In theory not necessary
          portYIELD_FROM_ISR(pdTRUE);
        }
      }
      else {
        xQueueSend(mQueue, aChunkStart, aBlocks ? portMAX_DELAY : 0);
      }
    }

    /// Removes the oldest chunk from the queue.
    static bool pop(char * const aChunkStart) noexcept {
      auto ret = xQueueReceive(mQueue, aChunkStart, pdMS_TO_TICKS(mPauseLength));
      return ret == pdTRUE ? true : false; // TODO remove
    }

    /// Pauses execution for the period given in the constructor.
    static void pause() noexcept {
      vTaskDelay(pdMS_TO_TICKS(mPauseLength));
    }

    /// Transmits the data using the serial descriptor given in the constructor.
    /// @param buffer start of data
    /// @param length length of data
    /// @param aProgressFlag address of flag to be set on transmission end.
    static void transmit(const char * const aBuffer, LogSizeType const aLength, std::atomic<bool> *aProgressFlag) noexcept {
      for(LogSizeType i = 0u; i < aLength; i++) {
        ITM_SendChar(static_cast<uint32_t>(aBuffer[i]));
      }
      aProgressFlag->store(false);
    }

    /// Starts the timer after which a partially filled buffer should be sent.
    static void startRefreshTimer(std::atomic<bool> *aRefreshFlag) noexcept {
      sRefreshNeeded = aRefreshFlag;
      xTimerStart(mRefreshTimer, 0);
    }

    /// Sets the flag.
    static void refreshNeeded() noexcept {
      sRefreshNeeded->store(true);
    }

    /// Calls az OS-specific lock to acquire a critical section, if implemented
    static void lock() noexcept {
      xSemaphoreTakeFromISR(mApiGuard, nullptr);
    }

    /// Calls az OS-specific lock to release critical section, if implemented
    static void unlock() noexcept {
      xSemaphoreGiveFromISR(mApiGuard, nullptr);
    }
  };

} //namespace nowtech

#endif // NOWTECH_LOGFREERTOSCMSISSWO_H_INCLUDED



















