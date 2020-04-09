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

#ifndef NOWTECH_LOG_NOP_INCLUDED
#define NOWTECH_LOG_NOP_INCLUDED

#include "Log.h"

namespace nowtech {

/// Class implementing a no-operation log interface.
class LogNop final : public LogOsInterface {
public:
  /// Does nothing.
  LogNop(LogConfig const & aConfig) noexcept
    : LogOsInterface(aConfig) {
  }

  /// This object is not intended to be deleted, so control should never
  /// get here.
  virtual ~LogNop() {
  }

  /// @return nullptr.
  virtual char const * getCurrentThreadName() noexcept {
    return "";
  }

  virtual char const *getThreadName(uint32_t const aHandle) noexcept {
    return nullptr;
  };

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

  /// Does nothing.
  virtual void push(char const * const aChunkStart, bool const) noexcept {
  }

  /// Does nothing.
  virtual bool pop(char * const aChunkStart) noexcept {
    return true;
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

#endif // NOWTECH_LOG_NOP_INCLUDED
