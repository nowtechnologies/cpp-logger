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

// clang++ -std=c++20 -Isrc -Itest test/test-stdostream.cpp -lpthread -o test-stdostream

constexpr int32_t threadCount = 10;

char names[10][10] = {
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
}

constexpr bool cgLogFromIsr = false;
constexpr bool cgArchitecture64 = true;
constexpr uint8_t cgAppendStackBufferSize = 100u;
constexpr bool cgAppendBasePrefix = true;
constexpr bool cgAlignSigned = true;
constexpr size_t cgTransmitBufferSize = 123u;
constexpr size_t cgPayloadSize = 8u;
constexpr size_t cgQueueSize = 8u;
constexpr nowtech::log::LogTopic cgMaxTopicCount = 1;
constexpr nowtech::log::TaskRepresentation cgTaskRepresentation = nowtech::log::TaskRepresentation::cName;
constexpr size_t cgDirectBufferSize = 43u;

using LogAppInterfaceStd = nowtech::log::AppInterfaceStd<cgMaxTopicCount, cgLogFromIsr>;
constexpr typename LogAppInterfaceStd::LogTime cgTimeout = 123u;
constexpr typename LogAppInterfaceStd::LogTime cgRefreshPeriod = 444;
using LogConverterCustomText = nowtech::log::ConverterCustomText<cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSenderStdOstream = nowtech::log::SenderStdOstream<LogAppInterfaceStd, LogConverterCustomText, cgTransmitBufferSize, cgTimeout, cgRefreshPeriod>;
using LogMessage = nowtech::log::MessageVariant<cgPayloadSize>;
using LogQueueVoid = nowtech::log::QueueVoid<LogMessage, cgQueueSize>;
using Log = nowtech::log::Log<LogQueueVoid, LogSenderStdOstream, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize>;
 
void delayedLog(int32_t n) {
  Log::i(nowtech::LogTopics::system) << n << ": " << 0 << Log::end;
  for(int64_t i = 1; i < 13; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1 << i));
    Log::i(nowtech::LogTopics::system) << n << ". thread delay logarithm: " << LC::X1 << i << Log::end;
  }
}

int main() {
  std::thread threads[threadCount];
  
  nowtech::log::LogConfig logConfig;
  logConfig.allowRegistrationLog = true;
  LogSenderStdOstream::init(&std::cout);
  Log::init(logConfig);

  Log::registerTopic(nowtech::LogTopics::system, "system");

  uint64_t const uint64 = 123456789012345;
  int64_t const int64 = -123456789012345;

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
  Log::i() << "float: " << -123.4567890f << Log::end;
  Log::i() << "double: " << 123.4567890 << Log::end;
  Log::i() << "float: " << -0.01234567890f << Log::end;
  Log::i() << "double: " << 0.01234567890 << Log::end;
  Log::i() << "bool:" << true << Log::end;
  Log::i() << "bool:" << false << Log::end;

  for(int32_t i = 0; i < threadCount; ++i) {
    threads[i] = std::thread(delayedLog, i);
  }
  for(int32_t i = 0; i < threadCount; ++i) {
    threads[i].join();
  }
  return 0;
}

