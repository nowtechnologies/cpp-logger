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
constexpr bool cgAllowRegistrationLog = true;
constexpr bool cgLogFromIsr = false;
constexpr size_t cgTaskShutdownSleepPeriod = 100u;
constexpr bool cgArchitecture64 = false;
constexpr uint8_t cgAppendStackBufferSize = 100u;
constexpr bool cgAppendBasePrefix = true;
constexpr bool cgAlignSigned = false;
using AtomicBufferType = int32_t;
constexpr size_t cgAtomicBufferExponent = 14u;
constexpr AtomicBufferType cgAtomicBufferInvalidValue = 1234546789;
constexpr size_t cgTransmitBufferSize = 123u;
constexpr size_t cgPayloadSize = 6u;
constexpr bool cgSupportFloatingPoint = false;
constexpr size_t cgQueueSize = 111u;
constexpr nowtech::log::LogTopic cgMaxTopicCount = 2;
constexpr nowtech::log::TaskRepresentation cgTaskRepresentation = nowtech::log::TaskRepresentation::cId;
constexpr nowtech::log::ErrorLevel cgErrorLevel = nowtech::log::ErrorLevel::Error;

constexpr uint32_t cgLogTaskStackSize = 256u;
constexpr uint32_t cgLogTaskPriority = tskIDLE_PRIORITY + 1u;
using LogAppInterface = nowtech::log::AppInterfaceFreeRtosMinimal<cgMaxTaskCount, cgLogFromIsr, cgTaskShutdownSleepPeriod>;
constexpr typename LogAppInterface::LogTime cgTimeout = 123u;
constexpr typename LogAppInterface::LogTime cgRefreshPeriod = 444;

//using LogAtomicBuffer = nowtech::log::AtomicBufferVoid;
using LogAtomicBuffer = nowtech::log::AtomicBufferOperational<LogAppInterface, AtomicBufferType, cgAtomicBufferExponent, cgAtomicBufferInvalidValue>;

constexpr size_t cgDirectBufferSize = 43u;
using LogMessage = nowtech::log::MessageCompact<cgPayloadSize, cgSupportFloatingPoint>;
using LogConverter = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderStmHalMinimal<LogAppInterface, LogConverter, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueVoid<LogMessage, LogAppInterface>;
using LogConfig = nowtech::log::Config<cgAllowRegistrationLog, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod, cgErrorLevel>;
using Log = nowtech::log::Log<LogQueue, LogSender, LogAtomicBuffer, LogConfig>;
/*   text    data     bss     dec     hex filename
noat   11680       32   19076   30788    7844 cpp-logger-embedded.elf
atom   12008       32   19092   31132    799c cpp-logger-embedded.elf
onlyat 11172       28   19088   30288    7650 cpp-logger-embedded.elf
 */

/* No log call present at all, not even cout.write, only thread creation:
   text    data     bss     dec     hex filename
   7468      24   19016   26508    678c cpp-logger-embedded.elf
*/

/*constexpr size_t cgDirectBufferSize = 43u;
using LogMessage = nowtech::log::MessageCompact<cgPayloadSize, cgSupportFloatingPoint>;
using LogConverter = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderVoid<LogAppInterface, LogConverter, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueVoid<LogMessage, LogAppInterface>;
using LogConfig = nowtech::log::Config<cgAllowRegistrationLog, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod, cgErrorLevel>;
using Log = nowtech::log::Log<LogQueue, LogSender, LogAtomicBuffer, LogConfig>;
   text    data     bss     dec     hex filename
noat    7468       24   19016   26508    678c cpp-logger-embedded.elf
atom    7468       24   19016   26508    678c cpp-logger-embedded.elf
 */

/*constexpr size_t cgDirectBufferSize = 0u;
using LogMessage = nowtech::log::MessageVariant<cgPayloadSize, cgSupportFloatingPoint>;
using LogConverter = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderStmHalMinimal<LogAppInterface, LogConverter, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueFreeRtos<LogMessage, LogAppInterface, cgQueueSize>;
using LogConfig = nowtech::log::Config<cgAllowRegistrationLog, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod, cgErrorLevel>;
using Log = nowtech::log::Log<LogQueue, LogSender, LogAtomicBuffer, LogConfig>;
    text    data     bss     dec     hex filename
noat   13880       32   19100   33012    80f4 cpp-logger-embedded.elf
atom   14608       32   19124   33764    83e4 cpp-logger-embedded.elf
*/

/*constexpr size_t cgDirectBufferSize = 0u;
using LogMessage = nowtech::log::MessageCompact<cgPayloadSize, cgSupportFloatingPoint>;
using LogConverter = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderStmHalMinimal<LogAppInterface, LogConverter, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueFreeRtos<LogMessage, LogAppInterface, cgQueueSize>;
using LogConfig = nowtech::log::Config<cgAllowRegistrationLog, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod, cgErrorLevel>;
using Log = nowtech::log::Log<LogQueue, LogSender, LogAtomicBuffer, LogConfig>;
   text    data     bss     dec     hex filename
noat   13608       32   19100   32740    7fe4 cpp-logger-embedded.elf
atom   14336       32   19124   33492    82d4 cpp-logger-embedded.elf
onlyat 13676       28   19120   32824    8038 cpp-logger-embedded.elf
 */

void step() {
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
  vTaskDelay(133);
}

uint32_t cgUint32;
int32_t cgInt32;
char constexpr cgString[] = "string\n";

extern "C" void myDefaultTask() {
  nowtech::log::LogFormatConfig logConfig;
  LogSender::init(&huart1);
  Log::init(logConfig, cgLogTaskStackSize, cgLogTaskPriority);

//  Log::registerTopic(nowtech::LogTopics::system, "system");
//  Log::registerTopic(nowtech::LogTopics::surplus, "surplus");
  Log::registerCurrentTask("main");

  /*Log::i(nowtech::LogTopics::surplus) << cgUint32 << Log::end;
  Log::i(nowtech::LogTopics::system) << cgInt32 << Log::end;
  Log::n() << cgString << Log::end;
  Log::n() << LC::St << cgString << Log::end;*/

  Log::pushAtomic(cgInt32);
  Log::sendAtomicBuffer();

  while(true) {
    step();
  }
}

extern "C" void vApplicationMallocFailedHook(void) {
  while(true) {
    step();
  }
}
