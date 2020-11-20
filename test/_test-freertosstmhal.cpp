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

#include "LogFreertosStmHal.h"

extern UART_HandleTypeDef huart3;

void pause(uint32_t const aMs) noexcept {
  vTaskDelay(pdMS_TO_TICKS(aMs));
}

/*
the first functions require these in Log:
    static void push(char const * const aChunk) {
      sInstance->doPush(aChunk);
    }

    void doPush(char const * const aChunk) {
      mOsInterface.push(aChunk, true);
    }
    */

void testInterleave() noexcept {
  static nowtech::LogConfig config;
  config.chunkSize = 8u;
  config.queueLength = 64u;
  config.circularBufferLength = 64u;
  config.transmitBufferLength = 32u;
  config.appendStackBufferLength = 34u;
  config.pauseLength = 10u;
  config.refreshPeriod = 1000u;
  constexpr uint32_t logTaskStack = 512;
  static nowtech::LogFreeRtosStmHal osInterface(&huart3, config, logTaskStack, configMAX_PRIORITIES / 2);
  static Log log(osInterface, config);
  Log::registerCurrentTask();

  /*Log::push("1aaa\n");
  Log::push("2bbb\n");
  Log::push("1a123456");
  Log::push("2bABCDEF");
  Log::push("1a789012");
  Log::push("2bGHIJKL");
  Log::push("1a345678");
  Log::push("2bMNOPQR");
  Log::push("1a90\n");
  Log::push("2bST\n");
  Log::push("1aaa\n");
  Log::push("2bbb\n");
  Log::push("1a123456");
  Log::push("2bABCDEF");
  Log::push("1a78\n");
  Log::push("2bGHI\n");*/
}

void testInterleave2() noexcept {
  static nowtech::LogConfig config;
  config.chunkSize = 8u;
  config.queueLength = 64u;
  config.circularBufferLength = 64u;
  config.transmitBufferLength = 32u;
  config.appendStackBufferLength = 34u;
  config.pauseLength = 10u;
  config.refreshPeriod = 1000u;
  constexpr uint32_t logTaskStack = 512;
  static nowtech::LogFreeRtosStmHal osInterface(&huart3, config, logTaskStack, configMAX_PRIORITIES / 2);
  static Log log(osInterface, config);
  Log::registerCurrentTask();

 /* Log::push("1aaa\n");
  Log::push("2bbb\n");
  Log::push("1a123456");
  Log::push("2bABCDEF");
  Log::push("1a789012");
  Log::push("2bGHIJKL");
  Log::push("3cfghjkl");
  Log::push("3cmnopqr");
  Log::push("3cstuv\n");
  Log::push("1a345678");
  Log::push("2bMNOPQR");
  Log::push("1a90\n");
  Log::push("2bST\n");
  Log::push("1aaa\n");
  Log::push("2bbb\n");
  Log::push("1a123456");
  Log::push("2bABCDEF");
  Log::push("1a78\n");
  Log::push("2bGHI\n");
  Log::push("1aaa\n");
  Log::push("2bbb\n");
  Log::push("1aaa\n");
  Log::push("2bbb\n");*/
}

void testInterleaveFullCircular() noexcept {
  static nowtech::LogConfig config;
  config.chunkSize = 8u;
  config.queueLength = 64u;
  config.circularBufferLength = 2u;
  config.transmitBufferLength = 32u;
  config.appendStackBufferLength = 34u;
  config.pauseLength = 10u;
  config.refreshPeriod = 1000u;
  constexpr uint32_t logTaskStack = 512;
  static nowtech::LogFreeRtosStmHal osInterface(&huart3, config, logTaskStack, configMAX_PRIORITIES / 2);
  static Log log(osInterface, config);
  Log::registerCurrentTask();

 /* Log::push("1aaa\n");   // 1
  Log::push("2bbb\n");   // 2
  Log::push("1a123456"); // 3
  Log::push("2bABCDEF"); // 5 forced to pop, ruins the one in progress
  Log::push("1a789012"); // 4
  Log::push("2bGHIJKL"); // 6
  Log::push("1a345678"); // 8
  Log::push("2bMNOPQR"); // 7
  Log::push("1a90\n");   // 9
  Log::push("2bST\n");  */ // 10
}

// needs Log::send("TX interrupt."); in HAL_UART_TxCpltCallback to test logging from ISR
void testRealTiming() noexcept {
  static nowtech::LogConfig config;
  config.chunkSize = 8u;
  config.queueLength = 64u;
  config.circularBufferLength = 64u;
  config.transmitBufferLength = 32u;
  config.appendStackBufferLength = 34u;
  config.pauseLength = 10u;
  config.refreshPeriod = 1000u;
  constexpr uint32_t logTaskStack = 512;
  static nowtech::LogFreeRtosStmHal osInterface(&huart3, config, logTaskStack, configMAX_PRIORITIES / 2);
  static Log log(osInterface, config);
  Log::registerCurrentTask();

  Log::send("aaa");
  pause(400u);
  Log::send("b123456789b123456789");
  pause(400u);
  Log::send("aaa ", static_cast<uint32_t>(123456789));
  pause(400u);
  Log::send("bbb");
  pause(400u);
  Log::send("aaa");
  pause(400u);
  Log::send("bbb");
}

void testFullTransmit() noexcept {
  static nowtech::LogConfig config;
  config.chunkSize = 8u;
  config.queueLength = 64u;
  config.circularBufferLength = 64u;
  config.transmitBufferLength = 4u;
  config.appendStackBufferLength = 34u;
  config.pauseLength = 10u;
  config.refreshPeriod = 1000u;
  constexpr uint32_t logTaskStack = 512;
  static nowtech::LogFreeRtosStmHal osInterface(&huart3, config, logTaskStack, configMAX_PRIORITIES / 2);
  static Log log(osInterface, config);
  Log::registerCurrentTask();

  Log::send("aaa");
  pause(100u);
  Log::send("b123456789b123456789");
  pause(100u);
  Log::send("aaa ", static_cast<uint32_t>(123456789));
  pause(100u);
  Log::send("bbb");
  pause(100u);
  Log::send("aaa");
  pause(100u);
  Log::send("bbb");
}

void testDefaultFormat() noexcept {
  static nowtech::LogConfig config;
  config.chunkSize = 8u;
  config.queueLength = 64u;
  config.circularBufferLength = 64u;
  config.transmitBufferLength = 32u;
  config.appendStackBufferLength = 34u;
  config.pauseLength = 10u;
  config.refreshPeriod = 1000u;
  constexpr uint32_t logTaskStack = 512;
  static nowtech::LogFreeRtosStmHal osInterface(&huart3, config, logTaskStack, configMAX_PRIORITIES / 2);
  static Log log(osInterface, config);

  Log::registerCurrentTask();
  Log::send("int8: ", static_cast<int8_t>(123));
  Log::send("int16: ", static_cast<int16_t>(123));
  Log::send("int32: ", static_cast<int32_t>(123));
  Log::send("uint8: ", static_cast<uint8_t>(123));
  Log::send("uint16: ", static_cast<uint16_t>(123));
  Log::send("uint32: ", static_cast<uint32_t>(123));
  Log::send("float: ", 1.234567890f);
  Log::send("double: ", -1.234567890);
  Log::send("float: ", -123.4567890f);
  Log::send("double: ", 123.4567890);
  Log::send("float: ", -0.01234567890f);
  Log::send("double: ", 0.01234567890);
  Log::send("bool:", true);
  Log::send("bool:", false);
}

void testCustomFormat() noexcept {
  static nowtech::LogConfig config;
  config.chunkSize = 8u;
  config.queueLength = 64u;
  config.circularBufferLength = 64u;
  config.transmitBufferLength = 32u;
  config.appendStackBufferLength = 34u;
  config.pauseLength = 10u;
  config.refreshPeriod = 1000u;
  config.taskRepresentation = LC::TaskRepresentation::cName;
  config.tickFormat   = LC::cNone;
  config.int8Format   = LC::cX2;
  config.int16Format  = LC::cX4;
  config.int32Format  = LC::cX8;
  config.uint8Format  = LC::cX2;
  config.uint16Format = LC::cX4;
  config.uint32Format = LC::cX8;
  config.floatFormat  = LC::cD2;
  config.doubleFormat = LC::cD3;
  constexpr uint32_t logTaskStack = 512;
  static nowtech::LogFreeRtosStmHal osInterface(&huart3, config, logTaskStack, configMAX_PRIORITIES / 2);
  static Log log(osInterface, config);

  Log::registerCurrentTask();
  Log::send("int8: ", static_cast<int8_t>(123));
  Log::send("int16: ", static_cast<int16_t>(123));
  Log::send("int32: ", static_cast<int32_t>(123));
  Log::send("uint8: ", static_cast<uint8_t>(123));
  Log::send("uint16: ", static_cast<uint16_t>(123));
  Log::send("uint32: ", static_cast<uint32_t>(123));
  Log::send("float: ", 1.234567890f);
  Log::send("double: ", -1.234567890);
  Log::send("float: ", -123.4567890f);
  Log::send("double: ", 123.4567890);
  Log::send("float: ", -0.01234567890f);
  Log::send("double: ", 0.01234567890);
}

void testCustomFormat2() noexcept {
  static nowtech::LogConfig config;
  config.chunkSize = 8u;
  config.queueLength = 64u;
  config.circularBufferLength = 64u;
  config.transmitBufferLength = 32u;
  config.appendStackBufferLength = 34u;
  config.pauseLength = 10u;
  config.refreshPeriod = 1000u;
  constexpr uint32_t logTaskStack = 512;
  static nowtech::LogFreeRtosStmHal osInterface(&huart3, config, logTaskStack, configMAX_PRIORITIES / 2);
  static Log log(osInterface, config);

  Log::registerCurrentTask();
  Log::send("int8: ", LC::cB8, static_cast<int8_t>(-123));
  Log::send("int16: ", LC::cB16, static_cast<int16_t>(-123));
  Log::send("int32: ", LC::cB32, static_cast<int32_t>(-123));
  Log::send("uint8: ", LC::cB8, static_cast<uint8_t>(123));
  Log::send("uint16: ", LC::cB16, static_cast<uint16_t>(123));
  Log::send("uint32: ", LC::cB32, static_cast<uint32_t>(123));
  Log::send("float: ", LC::cD2, 1.234f);
  Log::send("double: ", LC::cD2, -1.234);
  Log::send("float: ", LC::cD2, -125.0f);
  Log::send("double: ", LC::cD2, 125.0);
  Log::send("float: ", LC::cD2, -0.012567890f);
  Log::send("double: ", LC::cD2, 0.012567890);
  Log::send("bool:", true);
  Log::send("bool:", false);
}

void testPerformance() noexcept {
  static nowtech::LogConfig config;
  config.chunkSize = 8u;
  config.queueLength = 64u;
  config.circularBufferLength = 64u;
  config.transmitBufferLength = 32u;
  config.appendStackBufferLength = 34u;
  config.pauseLength = 10u;
  config.refreshPeriod = 1000u;
  config.taskRepresentation = LC::TaskRepresentation::cNone;
  config.tickFormat   = LC::cNone;
  constexpr uint32_t logTaskStack = 512;
  static nowtech::LogFreeRtosStmHal osInterface(&huart3, config, logTaskStack, configMAX_PRIORITIES / 2);
  static Log log(osInterface, config);

  Log::registerCurrentTask();
  for(int i = 0; i < 200; i++) {
    for(int j = 0; j < 10; j++) {
      Log::send("now: ", LC::cD5, static_cast<int32_t>(i * 10 + j));
    }
    pause(10);
  }
}

void testPerformanceBlocks() noexcept {
  static nowtech::LogConfig config;
  config.chunkSize = 8u;
  config.queueLength = 64u;
  config.circularBufferLength = 64u;
  config.transmitBufferLength = 32u;
  config.appendStackBufferLength = 34u;
  config.pauseLength = 10u;
  config.refreshPeriod = 1000u;
  config.taskRepresentation = LC::TaskRepresentation::cNone;
  config.tickFormat   = LC::cNone;
  constexpr uint32_t logTaskStack = 512;
  static nowtech::LogFreeRtosStmHal osInterface(&huart3, config, logTaskStack, configMAX_PRIORITIES / 2);
  static Log log(osInterface, config);

  Log::registerCurrentTask();
  for(int i = 0; i < 200; i++) {
    for(int j = 0; j < 10; j++) {
      Log::send("now: ", LC::cD5, static_cast<int32_t>(i * 10 + j));
    }
    pause(5);
  }
}

void testPerformanceDrops() noexcept {
  static nowtech::LogConfig config;
  config.chunkSize = 8u;
  config.queueLength = 64u;
  config.circularBufferLength = 64u;
  config.transmitBufferLength = 32u;
  config.appendStackBufferLength = 34u;
  config.pauseLength = 10u;
  config.refreshPeriod = 1000u;
  config.taskRepresentation = LC::TaskRepresentation::cNone;
  config.tickFormat   = LC::cNone;
  config.blocks = false;
  constexpr uint32_t logTaskStack = 512;
  static nowtech::LogFreeRtosStmHal osInterface(&huart3, config, logTaskStack, configMAX_PRIORITIES / 2);
  static Log log(osInterface, config);

  Log::registerCurrentTask();
  for(int i = 0; i < 200; i++) {
    for(int j = 0; j < 10; j++) {
      Log::send("now: ", LC::cD5, static_cast<int32_t>(i * 10 + j));
    }
    pause(2);
  }
}

void testLog() noexcept {
  //testInterleave();
  //testInterleave2();
  //testInterleaveFullCircular();
  testRealTiming();
  //testFullTransmit();
  //testDefaultFormat();
  //testCustomFormat2();
  //testPerformance();
  //testPerformanceBlocks();
  //testPerformanceDrops();
  while(true) {
  }
}
