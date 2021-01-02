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
constexpr bool cgSupportFloatingPoint = true;
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
noat   22180      132   19072   41384    a1a8 cpp-logger-embedded.elf
atom   22508      132   19096   41736    a308 cpp-logger-embedded.elf
new    22508      132   19096   41736    a308 cpp-logger-embedded.elf
 */

/* No log call present at all, not even cout.write, only thread creation:
   text    data     bss     dec     hex filename
   9108      24   19016   28148    6df4 cpp-logger-embedded.elff
*/


/*constexpr size_t cgDirectBufferSize = 43u;
using LogMessage = nowtech::log::MessageCompact<cgPayloadSize, cgSupportFloatingPoint>;
using LogConverter = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderVoid<LogAppInterface, LogConverter, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueVoid<LogMessage, LogAppInterface>;
using LogConfig = nowtech::log::Config<cgAllowRegistrationLog, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod, cgErrorLevel>;
using Log = nowtech::log::Log<LogQueue, LogSender, LogAtomicBuffer, LogConfig>;
   text    data     bss     dec     hex filename
noat    9108       24   19016   28148    6df4 cpp-logger-embedded.elf
atom    9108       24   19016   28148    6df4 cpp-logger-embedded.elf
 */

/*constexpr size_t cgDirectBufferSize = 0u;
using LogMessage = nowtech::log::MessageVariant<cgPayloadSize, cgSupportFloatingPoint>;
using LogConverter = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderStmHalMinimal<LogAppInterface, LogConverter, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueFreeRtos<LogMessage, LogAppInterface, cgQueueSize>;
using LogConfig = nowtech::log::Config<cgAllowRegistrationLog, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod, cgErrorLevel>;
using Log = nowtech::log::Log<LogQueue, LogSender, LogAtomicBuffer, LogConfig>;
    text    data     bss     dec     hex filename
noat   24380      132   19104   43616    aa60 cpp-logger-embedded.elf
atom   25108      132   19128   44368    ad50 cpp-logger-embedded.elf
*/

/*constexpr size_t cgDirectBufferSize = 0u;
using LogMessage = nowtech::log::MessageCompact<cgPayloadSize, cgSupportFloatingPoint>;
using LogConverter = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderStmHalMinimal<LogAppInterface, LogConverter, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueFreeRtos<LogMessage, LogAppInterface, cgQueueSize>;
using LogConfig = nowtech::log::Config<cgAllowRegistrationLog, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod, cgErrorLevel>;
using Log = nowtech::log::Log<LogQueue, LogSender, LogAtomicBuffer, LogConfig>;
   text    data     bss     dec     hex filename
noat   24092      132   19104   43328    a940 cpp-logger-embedded.elf
atom   24820      132   19128   44080    ac30 cpp-logger-embedded.elf
 */

void step() {
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
  vTaskDelay(133);
}

uint32_t cgUint32;
int32_t cgInt32;
char constexpr cgString[] = "string\n";

extern "C" void myDefaultTask() {
  float real = xTaskGetTickCount() * xTaskGetTickCount(); // Use some common operations to create a bias on floating point arithmetics in converter.
  real = xTaskGetTickCount() / real;
  if(real < xTaskGetTickCount() || real == xTaskGetTickCount() || static_cast<uint32_t>(real) > xTaskGetTickCount()) {
    real = -real;
  }
  else { // nothing to do
  }
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, real > 0.0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  nowtech::log::LogFormatConfig logConfig;
  LogSender::init(&huart1);
  Log::init(logConfig, cgLogTaskStackSize, cgLogTaskPriority);

  Log::registerTopic(nowtech::LogTopics::system, "system");
  Log::registerTopic(nowtech::LogTopics::surplus, "surplus");
  Log::registerCurrentTask("main");

  Log::i(nowtech::LogTopics::surplus) << cgUint32 << Log::end;
  Log::i(nowtech::LogTopics::system) << cgInt32 << Log::end;
  Log::n() << real << cgString << Log::end;
  Log::n() << real << LC::St << cgString << Log::end;

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
