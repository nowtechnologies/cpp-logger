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

#ifndef NOWTECH_LOG_CMSISSWO_INCLUDED
#define NOWTECH_LOG_CMSISSWO_INCLUDED

#include "Log.h"
#include "stm32utils.h"
#include "cmsis_os.h"  // we use only an assembly macro, nothing more CMSIS-related

namespace nowtech {

  /// Class implementing log interface for CMSIS SWO alone without any buffering or concurrency support.
  /// The ITM_SendChar contains a busy wait loop without timeout. If this is an issue,
  /// the function should be implemented with a timeout.
  class LogCmsisSwo final : public LogOsInterface {
  public:
    /// Sets parameters and creates the mutex for locking.
    /// @param aConfig config.
    LogCmsisSwo(LogConfig const & aConfig) noexcept
      : LogOsInterface(aConfig) {
    }

    /// This object is not intended to be deleted, so control should never
    /// get here.
    virtual ~LogCmsisSwo() {
    }

    /// Returns true if we are in an ISR.
    virtual bool isInterrupt() noexcept override {
      return stm32utils::isInterrupt();
    }

    /// Returns empty string.
    virtual const char * const getThreadName(uint32_t const aHandle) noexcept {
      return "";
    }

    /// Returns nullptr.
    virtual const char * const getCurrentThreadName() noexcept {
      return nullptr;
    }

    /// Returns 0.
    virtual uint32_t getCurrentThreadId() noexcept {
      return 0u;
    }

    /// Returns 0.
    virtual uint32_t getLogTime() const noexcept {
      return 0u;
    }

    /// Does nothing.
    virtual void createTransmitterThread(Log *, void(*)(void *)) noexcept {
    }

    /// Sends the chunk contents immediately.
    virtual void push(char const * const aChunkStart, bool const) noexcept {
      LogSizeType length;
      for(length = 0; length < mChunkSize - 1; ++length) {
        ITM_SendChar(static_cast<uint32_t>(aChunkStart[length + 1]));
        if(aChunkStart[length + 1] == '\n') {
          ++length;
          break;
        }
      }
    }

    /// Does nothing.
    virtual bool pop(char * const aChunkStart) noexcept {
      return false;
    }

    /// Does nothing.
    virtual void pause() noexcept {
    }

    /// Does nothing.
    virtual void transmit(const char * const, LogSizeType const, std::atomic<bool> *) noexcept {
    }

    /// Does nothing.
    static void transmitFinished() noexcept {
    }

    /// Does nothing.
    virtual void startRefreshTimer(std::atomic<bool> *) noexcept {
    }

    /// Does nothing.
    static void refreshNeeded() noexcept {
    }

  };

} //namespace nowtech

#endif // NOWTECH_LOG_CMSISSWO_INCLUDED
