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

#include "LogAppInterfaceStd.h"
#include "LogConverterCustomText.h"
#include "LogSenderStdOstream.h"
#include "LogQueueVoid.h"
#include "LogMessageVariant.h"
#include "Log.h"

#include <iostream>
#include <thread>

// clang++ -std=c++20 -Isrc -Icpp-memory-manager test/test-stdostream.cpp -lpthread -o test-stdostream

constexpr size_t cgThreadCount = 1;

char cgThreadNames[10][10] = {
  "thread_0",
  "thread_1",
  "thread_2",
  "thread_3",
  "thread_4",
  "thread_5",
  "thread_6",
  "thread_7",
  "thread_8",
  "thread_9"
};

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
constexpr size_t cgPayloadSize = 14u;
constexpr bool cgSupportFloatingPoint = true;
constexpr size_t cgQueueSize = 444u;
constexpr nowtech::log::LogTopic cgMaxTopicCount = 2;
constexpr nowtech::log::TaskRepresentation cgTaskRepresentation = nowtech::log::TaskRepresentation::cName;
constexpr size_t cgDirectBufferSize = 43u;
constexpr nowtech::log::ErrorLevel cgErrorLevel = nowtech::log::ErrorLevel::Error;

using LogAppInterface = nowtech::log::AppInterfaceStd<cgMaxTaskCount, cgLogFromIsr, cgTaskShutdownSleepPeriod>;
constexpr typename LogAppInterface::LogTime cgTimeout = 123u;
constexpr typename LogAppInterface::LogTime cgRefreshPeriod = 444;
using LogMessage = nowtech::log::MessageVariant<cgPayloadSize, cgSupportFloatingPoint>;
using LogConverterCustomText = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSenderStdOstream = nowtech::log::SenderStdOstream<LogAppInterface, LogConverterCustomText, cgTransmitBufferSize, cgTimeout>;
using LogQueueVoid = nowtech::log::QueueVoid<LogMessage, LogAppInterface, cgQueueSize>;
using LogAtomicBuffer = nowtech::log::AtomicBufferOperational<LogAppInterface, AtomicBufferType, cgAtomicBufferExponent, cgAtomicBufferInvalidValue>;
using LogConfig = nowtech::log::Config<cgAllowRegistrationLog, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod, cgErrorLevel>;
using Log = nowtech::log::Log<LogQueueVoid, LogSenderStdOstream, LogAtomicBuffer, LogConfig>;

void delayedLog(size_t n) {
  Log::registerCurrentTask(cgThreadNames[n]);
  Log::i(nowtech::LogTopics::system) << n << ": " << 0 << Log::end;
  for(int64_t i = 1; i < 3; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1 << i));
    Log::i(nowtech::LogTopics::system) << n << ". thread delay logarithm: " << LC::X1 << i << Log::end;
  }
  Log::unregisterCurrentTask();
}

std::atomic<int32_t> gCounter;
constexpr int32_t cgAtomicCount = 100;

void atomicLog(size_t n) {
  Log::registerCurrentTask(cgThreadNames[n]);
  for(int32_t i = 0; i < cgAtomicCount; ++i) {
    Log::pushAtomic(gCounter++);
  }
  Log::unregisterCurrentTask();
}

int main() {
  std::thread threads[cgThreadCount];
  
  nowtech::log::LogFormatConfig logConfig;
  LogSenderStdOstream::init(&std::cout);
  Log::init(logConfig);

  Log::registerTopic(nowtech::LogTopics::system, "system");
  Log::registerTopic(nowtech::LogTopics::surplus, "surplus");
  Log::registerCurrentTask("main");

  uint64_t const uint64 = 123456789012345;
  int64_t const int64 = -123456789012345;

  Log::i(nowtech::LogTopics::surplus) << "message" << Log::end;

  Log::i(nowtech::LogTopics::system) << "uint64: " << uint64 << " int64: " << int64 << Log::end;
  Log::n(nowtech::LogTopics::system) << "uint64: " << uint64 << " int64: " << int64 << Log::end;
  Log::i() << "uint64: " << uint64 << " int64: " << int64 << Log::end;
  Log::n() << "uint64: " << uint64 << " int64: " << int64 << Log::end;

  uint8_t const uint8 = 42;
  int8_t const int8 = -42;

  try {
    Log::i(nowtech::LogTopics::system) << uint8 << ' ' << int8 << Log::end;
    Log::i(nowtech::LogTopics::system) << LC::X2 << uint8 << ' ' << LC::D3 << int8 << Log::end;
    Log::i() << uint8 << ' ' << int8 << Log::end;
    Log::i() << LC::X2 << uint8 << int8 << Log::end;
    Log::i() << Log::end;
  }
  catch(std::exception &e) {
    Log::i() << "Exception: " << e.what() << Log::end;
  }

  Log::i() << "int8: " << static_cast<int8_t>(123) << Log::end;
  Log::i() << "int16: " << static_cast<int16_t>(123) << Log::end;
  Log::i() << "int32: " << static_cast<int32_t>(123) << Log::end;
  Log::i() << "int64: " << static_cast<int64_t>(123) << Log::end;
  Log::i() << "uint8: " << static_cast<uint8_t>(123) << Log::end;
  Log::i() << "uint16: " << static_cast<uint16_t>(123) << Log::end;
  Log::i() << "uint32: " << static_cast<uint32_t>(123) << Log::end;
  Log::i() << "uint64: " << static_cast<uint64_t>(123) << Log::end;
  Log::i() << "float: " << 1.234567890f << Log::end;
  Log::i() << "double: " << -1.234567890 << Log::end;
  Log::i() << "float: " << LC::Fm << -123.4567890f << Log::end;
  Log::i() << "double: " << LC::Fm << 123.4567890 << Log::end;
  Log::i() << "long double: " << -0.01234567890L << Log::end;
  Log::i() << "long double: " << LC::D16 << 0.01234567890L << Log::end;
  Log::i() << "bool:" << true << Log::end;
  Log::i() << "bool:" << false << Log::end;

  for(size_t i = 0; i < cgThreadCount; ++i) {
    threads[i] = std::thread(delayedLog, i);
  }
  for(size_t i = 0; i < cgThreadCount; ++i) {
    threads[i].join();
  }

  gCounter = 0;
  for(size_t i = 0; i < cgThreadCount; ++i) {
    threads[i] = std::thread(atomicLog, i);
  }
  for(size_t i = 0; i < cgThreadCount; ++i) {
    threads[i].join();
  }
  Log::sendAtomicBuffer();

  Log::unregisterCurrentTask();
  Log::done();
  return 0;
}

