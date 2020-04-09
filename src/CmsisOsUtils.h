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

#ifndef CMSIS_OS_UTILS_H_INCLUDED
#define CMSIS_OS_UTILS_H_INCLUDED

#include "FreeRTOS.h"
#include "task.h"

namespace nowtech {
  namespace OsUtil {
    bool sMemoryAllocationFailed;
    constexpr uint32_t cOneSecond = 1000u;

    uint32_t msToRtosTick(uint32_t const aTimeMs) noexcept;
    uint32_t rtosTickToMs(uint32_t const aTick) noexcept;
    uint32_t getUptimeMillis() noexcept;
    void taskDelayMillis(uint32_t const aTimeMs) noexcept;
    void infiniteWait() noexcept;
    bool isAnyMemoryAllocationFailed() noexcept;
  }
}

#endif // CMSIS_UTILS_H_INCLUDED






