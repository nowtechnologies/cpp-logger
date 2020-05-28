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

#include "Log.h"
#include "LogUtil.h"

void nowtech::push(char const mChar) noexcept {
}

constexpr nowtech::LogFormat nowtech::LogConfig::cDefault;
constexpr nowtech::LogFormat nowtech::LogConfig::cNone;
constexpr nowtech::LogFormat nowtech::LogConfig::cB8;
constexpr nowtech::LogFormat nowtech::LogConfig::cB16;
constexpr nowtech::LogFormat nowtech::LogConfig::cB24;
constexpr nowtech::LogFormat nowtech::LogConfig::cB32;
constexpr nowtech::LogFormat nowtech::LogConfig::cD2;
constexpr nowtech::LogFormat nowtech::LogConfig::cD3;
constexpr nowtech::LogFormat nowtech::LogConfig::cD4;
constexpr nowtech::LogFormat nowtech::LogConfig::cD5;
constexpr nowtech::LogFormat nowtech::LogConfig::cD6;
constexpr nowtech::LogFormat nowtech::LogConfig::cD7;
constexpr nowtech::LogFormat nowtech::LogConfig::cD8;
constexpr nowtech::LogFormat nowtech::LogConfig::cX2;
constexpr nowtech::LogFormat nowtech::LogConfig::cX4;
constexpr nowtech::LogFormat nowtech::LogConfig::cX6;
constexpr nowtech::LogFormat nowtech::LogConfig::cX8;

constexpr char nowtech::Log::cDigit2char[16];

nowtech::Log *nowtech::Log::sInstance;

void nowtech::Log::transmitterThreadFunction() noexcept
{
}

nowtech::Log::Log(LogOsInterface &aOsInterface, LogConfig const &aConfig) noexcept
  : tInterface(aOsInterface)
  , mConfig(aConfig)
  , mChunkSize(aConfig.mChunkSize) {
  sInstance = this;
}

void nowtech::Log::doRegisterCurrentTask() noexcept {
}

nowtech::TaskIdType nowtech::Log::getCurrentTaskId() const noexcept {
  return cInvalidTaskId;
}

// In the stub this will be implemented to return constant false
void nowtech::Log::startSend(Chunk &aChunk) noexcept {
}

void nowtech::Log::append(Chunk &aChunk, double const aValue, uint8_t const aDigitsNeeded) noexcept {
}

