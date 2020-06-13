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

#include "LogStdThreadOstream.h"
#include <iostream>
#include <cstdint>
#include <thread>

// clang++ -std=c++14 -Isrc src/Log.cpp src/LogStdThreadOstream.cpp src/LogUtil.cpp test/test-stdthreadostream.cpp -lpthread -g3 -Og -o test-stdthreadostream

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

namespace LogTopics {
nowtech::log::LogTopicInstance system;
}

/*void* operator new(std::size_t aCount) {
  std::cout << "::new(" << aCount << ")\n";
  return malloc(aCount);
}

void* operator new[]( std::size_t aCount ) {
  std::cout << "::new[](" << aCount << ")\n";
  return malloc(aCount);
}

void operator delete(void* aPtr) {
  std::cout << "::delete(void*)\n";
  free(aPtr);
}

void operator delete[](void* aPtr) {
  std::cout << "::delete[](void*)\n";
  free(aPtr);
}

void operator delete(void* aPtr, size_t) {
  std::cout << "::delete(void*, size_t)\n";
  free(aPtr);
}

void operator delete[](void* aPtr, size_t) {
  std::cout << "::delete[](void*, size_t)\n";
  free(aPtr);
}*/

class AppInterface final {
private:
  inline static char cExceptionTexts[static_cast<size_t>(nowtech::log::Exception::cCount)][32] = {
    "cOutOfTaskIds"
  };

public:
  static void fatalError(nowtech::log::Exception const aException) {
    std::cout << "fatal error: " << cExceptionTexts[static_cast<uint8_t>(aException)] << '\n';
  }

  template<typename tClass, typename ...tParameters>
  static tClass* _new(tParameters... aParameters) {
  std::cout << "_new\n";
    return ::new tClass(aParameters...);
  }

  template<typename tClass>
  static tClass* _newArray(uint32_t const aCount) {
  std::cout << "_newArray(" << aCount << ")\n";
    return ::new tClass[aCount];
  }

  template<typename tClass>
  static void _delete(tClass* aPointer) {
  std::cout << "_delete\n";
    ::delete aPointer;
  }

  template<typename tClass>
  static void _deleteArray(tClass* aPointer) {
  std::cout << "_deleteArray\n";
    ::delete[] aPointer;
  }
};

constexpr size_t  cChunkSize = 8u;
constexpr size_t  cMaxTaskCount = 18u;
constexpr size_t  cSizeofIntegerConversion = 8u;
constexpr uint8_t cAppendStackBufferLength = 80u;

typedef nowtech::log::LogStdThreadOstream<size_t, false, cChunkSize> LogInterface;
typedef nowtech::log::Log<AppInterface, LogInterface, cMaxTaskCount , cSizeofIntegerConversion, cAppendStackBufferLength> Log;
typedef nowtech::log::LogConfig LC;

void testArrayMap() noexcept {
  nowtech::log::ArrayMap<int16_t, double, 8u> map;
  map.insert(10, 10.1);
  map.insert(20, 20.1);
  map.insert(5, 5.1);
  map.insert(15, 15.1);
  map.insert(25, 25.1);
  map.insert(1, 1.1);
  map.insert(35, 35.1);
  map.insert(45, 45.1);
  map.insert(6, 6.1);
  map.insert(7, 7.1);
  std::cout << "ArrayMap.size() " << map.size() << '\n';
  std::cout << "ArrayMap.find(5) " << (map.find(5) != map.end()) << '\n';
  std::cout << "ArrayMap.find(8) " << (map.find(8) != map.end()) << '\n';
  for(auto &i : map) {
    std::cout << "ArrayMap " << i.key << ' ' << i.value << '\n';
  }
}
 
void delayedLog(int32_t n) {
  Log::registerCurrentTask(names[n]);
  Log::i(LogTopics::system) << n << ": " << 0 << Log::end;
  for(int64_t i = 1; i < 13; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1 << i));
    Log::i(LogTopics::system) << n << ". thread delay logarithm: " << LC::cX1 << i << Log::end;
  }
}

 
int main() {
  testArrayMap();

  std::thread threads[threadCount];
  
  nowtech::log::LogConfig logConfig;
  logConfig.taskRepresentation = nowtech::log::LogConfig::TaskRepresentation::cName;
  logConfig.refreshPeriod      = 200u;
  LogInterface::init(std::cout);
  Log::init(logConfig);
  Log::registerTopic(LogTopics::system, "system");

  uint64_t const uint64 = 123456789012345;
  int64_t const int64 = -123456789012345;

  Log::registerCurrentTask("main");

  int i = 33;
  Log::send(i);

  Log::send("send*");
  Log::send(LogTopics::system, LC::cX16, uint64);
  Log::send(uint64);
  Log::sendNoHeader(LogTopics::system, LC::cX16, uint64);
  Log::sendNoHeader(uint64);

  Log::send("<<");
  Log::i(LogTopics::system) << "uint64: " << uint64 << " int64: " << int64 << Log::end;
  Log::n(LogTopics::system) << "uint64: " << uint64 << " int64: " << int64 << Log::end;
  Log::i() << "uint64: " << uint64 << " int64: " << int64 << Log::end;
  Log::n() << "uint64: " << uint64 << " int64: " << int64 << Log::end;

  Log::i(LogTopics::system) << "uint64: " << LC::cX16 << uint64 << " int64: " << LC::cX16 << int64 << Log::end;
  Log::n(LogTopics::system) << "uint64: " << LC::cX16 << uint64 << " int64: " << LC::cX16 << int64 << Log::end;
  Log::i() << "uint64: " << LC::cX16 << uint64 << " int64: " << LC::cX16 << int64 << Log::end;
  Log::n() << "uint64: " << LC::cX16 << uint64 << " int64: " << LC::cX16 << int64 << Log::end;

  uint8_t const uint8 = 42;
  int8_t const int8 = -42;

  Log::send("small");
  Log::i(LogTopics::system) << uint8 << ' ' << int8 << Log::end;
  Log::i(LogTopics::system) << LC::cX2 << uint8 << ' ' << LC::cD3 << int8 << Log::end;
  Log::i() << uint8 << ' ' << int8 << Log::end;
  Log::i() << LC::cX2 << uint8 << int8 << Log::end;
  Log::i() << Log::end;

  Log::send("types");
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
  Log::i() << "long double: " << -1.234567890l << Log::end;
  Log::i() << "float: " << -123.4567890f << Log::end;
  Log::i() << "double: " << 123.4567890 << Log::end;
  Log::i() << "long double: " << 123.4567890l << Log::end;
  Log::i() << "float: " << -0.01234567890f << Log::end;
  Log::i() << "double: " << 0.01234567890 << Log::end;
  Log::i() << "long double: " << 0.01234567890l << Log::end;
  Log::i() << "bool:" << true << Log::end;
  Log::i() << "bool:" << false << Log::end;

  for(int32_t i = 0; i < threadCount; ++i) {
    threads[i] = std::thread(delayedLog, i);
  }
  for(int32_t i = 0; i < threadCount; ++i) {
    threads[i].join();
  }
  Log::done();
  return 0;
}


