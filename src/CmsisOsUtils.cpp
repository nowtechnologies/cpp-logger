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

#include "CmsisOsUtils.h"

bool nowtech::OsUtil::sMemoryAllocationFailed = false;

extern "C" void vApplicationMallocFailedHook(void) {
  nowtech::OsUtil::sMemoryAllocationFailed = true;
}

uint32_t nowtech::OsUtil::msToRtosTick(uint32_t const aTimeMs) noexcept {
  return aTimeMs / portTICK_PERIOD_MS;
}

uint32_t nowtech::OsUtil::rtosTickToMs(uint32_t const aTick) noexcept {
  return aTick * portTICK_PERIOD_MS;
}

uint32_t nowtech::OsUtil::getUptimeMillis() noexcept {
  return rtosTickToMs(xTaskGetTickCountFromISR());
}

void nowtech::OsUtil::taskDelayMillis(uint32_t const aTimeMs) noexcept {
  vTaskDelay(msToRtosTick(aTimeMs));
}

void nowtech::OsUtil::infiniteWait() noexcept {
  while(true) {
    taskDelayMillis(cOneSecond);
  }
}

bool nowtech::OsUtil::isAnyMemoryAllocationFailed() noexcept {
  return sMemoryAllocationFailed;
}




