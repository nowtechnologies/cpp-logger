//
// Created by balazs on 2020. 12. 03..
//

#include "LogAppInterfaceStd.h"
#include "LogConverterCustomText.h"
#include "LogSenderStdOstream.h"
#include "LogSenderVoid.h"
#include "LogQueueStdBoost.h"
#include "LogQueueVoid.h"
#include "LogMessageCompact.h"
#include "LogMessageVariant.h"
#include "Log.h"
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

// clang++ -std=c++20 -Isrc -Icpp-memory-manager -Os -Wl,-Map,test-sizes.map -demangle test/test-sizes-stdthreadostream.cpp -lpthread -o test-sizes
// clang++ -std=c++20 -Isrc -Icpp-memory-manager -Os -S -fno-asynchronous-unwind-tables -fno-dwarf2-cfi-asm -masm=intel test/test-sizes.cpp -o test-sizes.s
// llvm-size-10 test-sizes

constexpr size_t cgThreadCount = 1;

namespace nowtech::LogTopics {
nowtech::log::TopicInstance system;
nowtech::log::TopicInstance surplus;
}

constexpr nowtech::log::TaskId cgMaxTaskCount = cgThreadCount + 1;
constexpr bool cgAllowRegistrationLog = true;
constexpr bool cgLogFromIsr = false;
constexpr size_t cgTaskShutdownSleepPeriod = 100u;
constexpr bool cgArchitecture64 = true;
constexpr uint8_t cgAppendStackBufferSize = 100u;
constexpr bool cgAppendBasePrefix = true;
constexpr bool cgAlignSigned = false;
using AtomicBufferType = int32_t;
constexpr size_t cgAtomicBufferExponent = 14u;
constexpr AtomicBufferType cgAtomicBufferInvalidValue = 1234546789;
constexpr size_t cgTransmitBufferSize = 123u;
constexpr size_t cgPayloadSize = 8u;
constexpr bool cgSupportFloatingPoint = true;
constexpr size_t cgQueueSize = 444u;
constexpr nowtech::log::LogTopic cgMaxTopicCount = 2;
constexpr nowtech::log::TaskRepresentation cgTaskRepresentation = nowtech::log::TaskRepresentation::cName;
constexpr nowtech::log::ErrorLevel cgErrorLevel = nowtech::log::ErrorLevel::Error;

/*constexpr size_t cgDirectBufferSize = 43u;
using LogAppInterface = nowtech::log::AppInterfaceStd<cgMaxTaskCount, cgLogFromIsr, cgTaskShutdownSleepPeriod>;
constexpr typename LogAppInterface::LogTime cgTimeout = 123u;
constexpr typename LogAppInterface::LogTime cgRefreshPeriod = 444;
using LogMessage = nowtech::log::MessageVariant<cgPayloadSize, cgSupportFloatingPoint>;
using LogConverter = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderStdOstream<LogAppInterface, LogConverter, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueVoid<LogMessage, LogAppInterface>;
using LogAtomicBuffer = nowtech::log::AtomicBufferVoid;
using LogConfig = nowtech::log::Config<cgAllowRegistrationLog, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod, cgErrorLevel>;
using Log = nowtech::log::Log<LogQueue, LogSender, LogAtomicBuffer, LogConfig>;
   text	   data	    bss	    dec	    hex	filename
  15118	   1101	    496	  16715	   414b	test-sizes
*/

/* No log call present at all, not even cout.write, only thread creation:
   text	   data	    bss	    dec	    hex	filename
   3283	    828	      4	   4115	   1013	test-sizes
*/

/*constexpr size_t cgDirectBufferSize = 43u;
using LogAppInterface = nowtech::log::AppInterfaceStd<cgMaxTaskCount, cgLogFromIsr, cgTaskShutdownSleepPeriod>;
constexpr typename LogAppInterface::LogTime cgTimeout = 123u;
constexpr typename LogAppInterface::LogTime cgRefreshPeriod = 444;
using LogMessage = nowtech::log::MessageVariant<cgPayloadSize, cgSupportFloatingPoint>;
using LogConverter = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderVoid<LogAppInterface, LogConverter, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueVoid<LogMessage, LogAppInterface>;
using LogAtomicBuffer = nowtech::log::AtomicBufferVoid;
using LogConfig = nowtech::log::Config<cgAllowRegistrationLog, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod, cgErrorLevel>;
using Log = nowtech::log::Log<LogQueue, LogSender, LogAtomicBuffer, LogConfig>;
   text	   data	    bss	    dec	    hex	filename
   3283	    828	      4	   4115	   1013	test-sizes
 */

/*
constexpr size_t cgDirectBufferSize = 0u;
using LogAppInterface = nowtech::log::AppInterfaceStd<cgMaxTaskCount, cgLogFromIsr, cgTaskShutdownSleepPeriod>;
constexpr typename LogAppInterface::LogTime cgTimeout = 123u;
constexpr typename LogAppInterface::LogTime cgRefreshPeriod = 444;
using LogMessage = nowtech::log::MessageVariant<cgPayloadSize, cgSupportFloatingPoint>;
using LogConverter = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderStdOstream<LogAppInterface, LogConverter, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueStdBoost<LogMessage, LogAppInterface, cgQueueSize>;
using LogAtomicBuffer = nowtech::log::AtomicBufferVoid;
using LogConfig = nowtech::log::Config<cgAllowRegistrationLog, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod, cgErrorLevel>;
using Log = nowtech::log::Log<LogQueue, LogSender, LogAtomicBuffer, LogConfig>;
   text	   data	    bss	    dec	    hex	filename
  26086	   1285	    896	  28267	   6e6b	test-sizes
  */


constexpr size_t cgDirectBufferSize = 0u;
using LogAppInterface = nowtech::log::AppInterfaceStd<cgMaxTaskCount, cgLogFromIsr, cgTaskShutdownSleepPeriod>;
constexpr typename LogAppInterface::LogTime cgTimeout = 123u;
constexpr typename LogAppInterface::LogTime cgRefreshPeriod = 444;
using LogMessage = nowtech::log::MessageCompact<cgPayloadSize, cgSupportFloatingPoint>;
using LogConverter = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderStdOstream<LogAppInterface, LogConverter, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueStdBoost<LogMessage, LogAppInterface, cgQueueSize>;
using LogAtomicBuffer = nowtech::log::AtomicBufferVoid;
using LogConfig = nowtech::log::Config<cgAllowRegistrationLog, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod, cgErrorLevel>;
using Log = nowtech::log::Log<LogQueue, LogSender, LogAtomicBuffer, LogConfig>;
/*   text	   data	    bss	    dec	    hex	filename
  24458	   1285	    896	  26639	   680f	test-sizes
 */

uint32_t cgUint32 = 3;
int32_t cgInt32 = 4;
double constexpr cgReal = 3.3;
char constexpr cgString[] = "string\n";

void todo() noexcept {
  cgUint32 = getpid();
}

int main() {
  cgInt32 = getpid();
  std::thread thread(todo);
  thread.join();

  nowtech::log::LogFormatConfig logConfig;
  LogSender::init(&std::cout);
//  LogSender::init();
  Log::init(logConfig);

  Log::registerTopic(nowtech::LogTopics::system, "system");
  Log::registerTopic(nowtech::LogTopics::surplus, "surplus");
  Log::registerCurrentTask("main");

  Log::i(nowtech::LogTopics::surplus) << cgUint32 << Log::end;
  Log::i(nowtech::LogTopics::system) << cgInt32 << Log::end;
  Log::n() << cgReal << cgString << Log::end;
  Log::n() << cgReal << LC::St << cgString << Log::end;

  Log::unregisterCurrentTask();
  Log::done();

  return cgInt32 + cgUint32;
}

