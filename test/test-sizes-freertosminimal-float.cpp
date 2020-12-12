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

#include "LogAppInterfaceFreeRtosMinimal.h"
#include "LogConverterCustomText.h"
#include "LogSenderStmHalMinimal.h"
#include "LogSenderVoid.h"
#include "LogQueueFreeRtos.h"
#include "LogQueueVoid.h"
#include "LogMessageVariant.h"
#include "LogMessageCompact.h"
#include "Log.h"
#include "main.h"

extern UART_HandleTypeDef huart1;

namespace nowtech::LogTopics {
  nowtech::log::TopicInstance system;
  nowtech::log::TopicInstance surplus;
}

constexpr nowtech::log::TaskId cgMaxTaskCount = 1u;
constexpr bool cgLogFromIsr = false;
constexpr size_t cgTaskShutdownSleepPeriod = 100u;
constexpr bool cgArchitecture64 = false;
constexpr uint8_t cgAppendStackBufferSize = 100u;
constexpr bool cgAppendBasePrefix = true;
constexpr bool cgAlignSigned = false;
constexpr size_t cgTransmitBufferSize = 123u;
constexpr size_t cgPayloadSize = 8u;
constexpr size_t cgQueueSize = 111u;
constexpr nowtech::log::LogTopic cgMaxTopicCount = 2;
constexpr nowtech::log::TaskRepresentation cgTaskRepresentation = nowtech::log::TaskRepresentation::cName;
constexpr uint32_t cgLogTaskStackSize = 256u;
constexpr uint32_t cgLogTaskPriority = tskIDLE_PRIORITY + 1u;

/*constexpr size_t cgDirectBufferSize = 43u;
using LogAppInterfaceFreeRtosMinimal = nowtech::log::AppInterfaceFreeRtosMinimal<cgMaxTaskCount, cgLogFromIsr, cgTaskShutdownSleepPeriod>;
constexpr typename LogAppInterfaceFreeRtosMinimal::LogTime cgTimeout = 123u;
constexpr typename LogAppInterfaceFreeRtosMinimal::LogTime cgRefreshPeriod = 444;
using LogMessage = nowtech::log::MessageCompact<cgPayloadSize>;
using LogConverterCustomText = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderStmHalMinimal<LogAppInterfaceFreeRtosMinimal, LogConverterCustomText, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueVoid<LogMessage, LogAppInterfaceFreeRtosMinimal, cgQueueSize>;
using Log = nowtech::log::Log<LogQueue, LogSender, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod>;
   text    data     bss     dec     hex filename
  21988     132   19080   41200    a0f0 cpp-logger-embedded.elf
 */

/* No log call present at all, not even cout.write, only thread creation:
   text    data     bss     dec     hex filename
   7468      24   19016   26508    678c cpp-logger-embedded.elf
*/

/*constexpr size_t cgDirectBufferSize = 43u;
using LogAppInterfaceFreeRtosMinimal = nowtech::log::AppInterfaceFreeRtosMinimal<cgMaxTaskCount, cgLogFromIsr, cgTaskShutdownSleepPeriod>;
constexpr typename LogAppInterfaceFreeRtosMinimal::LogTime cgTimeout = 123u;
constexpr typename LogAppInterfaceFreeRtosMinimal::LogTime cgRefreshPeriod = 444;
using LogMessage = nowtech::log::MessageCompact<cgPayloadSize>;
using LogConverterCustomText = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderVoid<LogAppInterfaceFreeRtosMinimal, LogConverterCustomText, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueVoid<LogMessage, LogAppInterfaceFreeRtosMinimal, cgQueueSize>;
using Log = nowtech::log::Log<LogQueue, LogSender, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod>;
   text    data     bss     dec     hex filename
   7468      24   19016   26508    678c cpp-logger-embedded.elf
 */

/*constexpr size_t cgDirectBufferSize = 0u;
using LogAppInterfaceFreeRtosMinimal = nowtech::log::AppInterfaceFreeRtosMinimal<cgMaxTaskCount, cgLogFromIsr, cgTaskShutdownSleepPeriod>;
constexpr typename LogAppInterfaceFreeRtosMinimal::LogTime cgTimeout = 123u;
constexpr typename LogAppInterfaceFreeRtosMinimal::LogTime cgRefreshPeriod = 444;
using LogMessage = nowtech::log::MessageVariant<cgPayloadSize>;
using LogConverterCustomText = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderStmHalMinimal<LogAppInterfaceFreeRtosMinimal, LogConverterCustomText, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueFreeRtos<LogMessage, LogAppInterfaceFreeRtosMinimal, cgQueueSize>;
using Log = nowtech::log::Log<LogQueue, LogSender, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod>;
    text    data     bss     dec     hex filename
    24484     136   19104   43724    aacc cpp-logger-embedded.elf
*/

constexpr size_t cgDirectBufferSize = 0u;
using LogAppInterfaceFreeRtosMinimal = nowtech::log::AppInterfaceFreeRtosMinimal<cgMaxTaskCount, cgLogFromIsr, cgTaskShutdownSleepPeriod>;
constexpr typename LogAppInterfaceFreeRtosMinimal::LogTime cgTimeout = 123u;
constexpr typename LogAppInterfaceFreeRtosMinimal::LogTime cgRefreshPeriod = 444;
using LogMessage = nowtech::log::MessageCompact<cgPayloadSize>;
using LogConverterCustomText = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderStmHalMinimal<LogAppInterfaceFreeRtosMinimal, LogConverterCustomText, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueFreeRtos<LogMessage, LogAppInterfaceFreeRtosMinimal, cgQueueSize>;
using Log = nowtech::log::Log<LogQueue, LogSender, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod>;
/*
   text    data     bss     dec     hex filename
  24196     136   19104   43436    a9ac cpp-logger-embedded.elf
 */

void step() {
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
  vTaskDelay(133);
}

uint64_t cgUint32;
int64_t cgInt32;
double constexpr cgReal = 3.3;
char constexpr cgString[] = "string\n";

extern "C" void myDefaultTask() {
  nowtech::log::LogConfig logConfig;
  logConfig.allowRegistrationLog = true;
  LogSender::init(&huart1);
  Log::init(logConfig, cgLogTaskStackSize, cgLogTaskPriority);

  Log::registerTopic(nowtech::LogTopics::system, "system");
  Log::registerTopic(nowtech::LogTopics::surplus, "surplus");
  Log::registerCurrentTask("main");

  Log::i(nowtech::LogTopics::surplus) << cgUint32 << Log::end;
  Log::i(nowtech::LogTopics::system) << cgInt32 << Log::end;
  Log::n() << cgReal << cgString << Log::end;
  Log::n() << cgReal << LC::St << cgString << Log::end;

  while(true) {
    step();
  }
}

extern "C" void vApplicationMallocFailedHook(void) {
  while(true) {
    step();
  }
}
