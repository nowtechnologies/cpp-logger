/*
 * Copyright 2018 Now Technologies Zrt.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
 * THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef NOWTECH_LOG_STMHAL_INCLUDED
#define NOWTECH_LOG_STMHAL_INCLUDED

#include "Log.h"
#include "stm32hal.h"
#include "stm32utils.h"

namespace nowtech {

  /// Class implementing log interface for STM HAL alone without any buffering or concurrency support.
  class LogStmHal final : public LogOsInterface {
  private:
    /// The STM HAL serial port descriptor to use.
    UART_HandleTypeDef* mSerialDescriptor;

    /// Timeout for sending in ms
    uint32_t const mUartTimeout;

  public:
    /// Sets parameters and creates the mutex for locking.
    /// @param aSerialDescriptor the STM HAL serial port descriptor to use
    /// @param aConfig config.
    /// @param aUartTimeout timeout for sending perhaps in ms
    LogStmHal(UART_HandleTypeDef* const aSerialDescriptor
      , LogConfig const & aConfig
      , uint32_t const aUartTimeout) noexcept
      : LogOsInterface(aConfig)
      , mSerialDescriptor(aSerialDescriptor)
      , mUartTimeout(aUartTimeout) {
    }

    /// This object is not intended to be deleted, so control should never
    /// get here.
    static ~LogStmHal() {
    }

    /// Returns a textual representation of the current thread ID.
    /// This will be OS dependent.
    /// @return the thread ID text if called from a thread.
    static char const * getCurrentTaskName() noexcept {
      return "";
    }

    /// Returns a textual representation of the given thread ID.
    /// @return nullptr.
    static char const * getTaskName(uint32_t const aHandle) noexcept {
      return nullptr;
    };

    /// @return nullptr.
   /* static const char * const getCurrentTaskName() noexcept {
      return nullptr;
    }*/

    /// Returns 0.
    static uint32_t getCurrentTaskId() noexcept {
      return 0u;
    }

    /// Returns 0.
    static uint32_t getLogTime() const noexcept {
      return 0u;
    }

    /// Does nothing.
    static void createTransmitterTask(Log *, void(*)(void *)) noexcept {
    }

    /// Sends the chunk contents immediately.
    static void push(char const * const aChunkStart, bool const) noexcept {
      LogSizeType length;
      char buffer[mChunkSize];
      for(length = 0u; length < mChunkSize - 1u; ++length) {
        buffer[length] = aChunkStart[length + 1u];
        if(aChunkStart[length + 1] == '\n') {
          ++length;
          break;
        }
      }
      HAL_UART_Transmit(mSerialDescriptor, reinterpret_cast<uint8_t*>(const_cast<char*>(buffer)), length, mUartTimeout);
    }

    /// Does nothing.
    static bool pop(char * const aChunkStart) noexcept {
      return true;
    }

    /// Does nothing.
    static void pause() noexcept {
    }

    /// Does nothing.
    static void transmit(const char * const, LogSizeType const, std::atomic<bool> *) noexcept {
    }

    /// Does nothing.
    static void transmitFinished() noexcept {
    }

    /// Does nothing.
    static void startRefreshTimer(std::atomic<bool> *) noexcept {
    }

    /// Does nothing.
    static void refreshNeeded() noexcept {
    }

  };

} //namespace nowtech

#endif // NOWTECH_STMHAL_INCLUDED
