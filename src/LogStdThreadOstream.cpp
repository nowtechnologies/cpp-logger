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

#include "LogStdThreadOstream.h"

constexpr uint32_t nowtech::LogStdThreadOstream::cEnqueuePollDelay;

void nowtech::LogStdThreadOstream::FreeRtosQueue::send(char const * const aChunkStart, bool const aBlocks) noexcept {
  bool success;
  do {
    char *payload;
    success = mFreeList.pop(payload);
    if(success) {
      std::copy(aChunkStart, aChunkStart + mBlockSize, payload);
      mQueue.bounded_push(payload); // this should always succeed here
      mConditionVariable.notify_one();
    }
    else {
      std::this_thread::sleep_for(std::chrono::milliseconds(cEnqueuePollDelay));
    }
  } while(aBlocks && !success);
}

bool nowtech::LogStdThreadOstream::FreeRtosQueue::receive(char * const aChunkStart, uint32_t const mPauseLength) noexcept {
  bool result;
  // Safe to call empty because there will be only one consumer.
  if(mQueue.empty() && mConditionVariable.wait_for(mLock, std::chrono::milliseconds(mPauseLength)) == std::cv_status::timeout) {
    result = false;
  }
  else {
    char *payload;
    result = mQueue.pop(payload);
    if(result) {
      std::copy(payload, payload + mBlockSize, aChunkStart);
      mFreeList.bounded_push(payload); // this should always succeed here
    }
    else { // nothing to do
    }
  }
  return result;
}

void nowtech::LogStdThreadOstream::FreeRtosTimer::run() noexcept {
  while(mKeepRunning.load()) {
    if(mConditionVariable.wait_for(mLock, std::chrono::milliseconds(mTimeout)) == std::cv_status::timeout && mAlarmed.load()) {
      mLambda();
      mAlarmed.store(false);
    }
    else {
    }
  }
}

const char * nowtech::LogStdThreadOstream::getThreadName(uint32_t const aHandle) noexcept {
  char const * result = "";
  for(auto const &iterator : mTaskNamesIds) {
    if(iterator.second.id == aHandle) {
      result = iterator.second.name.c_str();
    }
    else { // nothing to do
    }
  }
  return result;
}

char const * nowtech::LogStdThreadOstream::getCurrentThreadName() noexcept {
  char const *result;
  auto found = mTaskNamesIds.find(std::this_thread::get_id());
  if(found != mTaskNamesIds.end()) {
    result = found->second.name.c_str();
  }
  else {
    result = Log::cUnknownApplicationName;
  }
  return result;
}

uint32_t nowtech::LogStdThreadOstream::getCurrentThreadId() noexcept {
  uint32_t result;
  auto found = mTaskNamesIds.find(std::this_thread::get_id());
  if(found != mTaskNamesIds.end()) {
    result = found->second.id;
  }
  else {
    result = cInvalidGivenTaskId;
  }
  return result;
}

