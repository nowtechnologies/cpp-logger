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
#include <ostream>

namespace nowtech {

  /// Class implementing log interface for STM HAL alone without any buffering or concurrency support.
  class LogStdOstream final : public LogOsInterface {
  private:
    /// The output stream to use.
    std::ostream &mOutput;

  public:
    /// The class does not own the stream and only writes to it.
    /// Opening and closing it is user responsibility.
    /// @param aOutput the std::ostream to use
    /// @param aConfig config.
    LogStdOstream(std::ostream &aOutput
      , LogConfig const & aConfig)
      : LogOsInterface(aConfig)
      , mOutput(aOutput) {
    }

    static ~LogStdOstream() {
      mOutput->flush();
    }
    
    /// Returns a textual representation of the current thread ID.
    /// This will be OS dependent.
    /// @return the thread ID text if called from a thread.
    static char const * getCurrentThreadName() noexcept {
      return "";
    }

    /// Returns a textual representation of the given thread ID.
    /// @return nullptr.
    static char const * getThreadName(uint32_t const aHandle) noexcept {
      return nullptr;
    };

    /// Returns 0.
    static uint32_t getCurrentThreadId() noexcept {
      return 0u;
    }

    /// Returns 0.
    static uint32_t getLogTime() const noexcept {
      return 0u;
    }

    /// Does nothing.
    static void createTransmitterThread(Log *, void(*)(void *)) noexcept {
    }

    /// Sends the chunk contents immediately.
    static void push(char const * const aChunkStart, bool const) noexcept {
      LogSizeType length;
      char buffer[mChunkSize];
      bool newLine = false;
      for(length = 0u; length < mChunkSize - 1u; ++length) {
        buffer[length] = aChunkStart[length + 1u];
        if(aChunkStart[length + 1] == '\r') {
          newLine = true;
          ++length;
          break;
        }
      }
      mOutput->write(buffer, length);
      if(newLine) {
        mOutput << '\n';
      }
      else { // nothing to do
      }
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
