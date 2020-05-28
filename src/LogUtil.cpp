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
#include "LogUtil.h"

nowtech::Chunk const &nowtech::CircularBuffer::inspect(TaskIdType const aTaskId) noexcept {
  while(mInspectedCount < mCount && mFound.getTaskId() != aTaskId) {
    ++mInspectedCount;
    ++mFound;
  }
  if(mInspectedCount == mCount) {
    nowtech::Chunk source(&tInterface, mBuffer, mBufferLength);
    nowtech::Chunk destination(&tInterface, mBuffer, mBufferLength);
    source = mStuffStart.getData();
    destination = mStuffStart.getData();
    while(source.getData() != mStuffEnd.getData()) {
      if(destination.getTaskId() != nowtech::cInvalidTaskId) {
        if(source.getData() == destination.getData()) {
          ++source;
        }
        else { // nothing to do
        }
        ++destination;
      }
      else {
        if(source.getTaskId() == nowtech::cInvalidTaskId) {
          ++source;
        }
        else {
          destination = source;
          source.invalidate();
        }
      }
    }
    char *startRemoved = destination.getData();
    char *endRemoved = mStuffEnd.getData();
    if(startRemoved > endRemoved) {
      endRemoved += mChunkSize * mBufferLength;
    }
    else { // nothing to do
    }
    mCount -= (endRemoved - startRemoved) / mChunkSize;
    mStuffEnd = destination.getData();
    mInspected = true;
  }
  return mFound;
}

nowtech::TransmitBuffers &nowtech::TransmitBuffers::operator<<(nowtech::Chunk const &aChunk) noexcept {
  if(aChunk.getTaskId() != nowtech::cInvalidTaskId) {
    LogSizeType i = 1;
    char const * const origin = aChunk.getData();
    mWasTerminalChunk = false;
    char * buffer = mBuffers[mBufferToWrite];
    LogSizeType &index = mIndex[mBufferToWrite];
    while(!mWasTerminalChunk && i < mChunkSize) {
      buffer[index] = origin[i];
      if (origin[i] == cEndOfMessage) {
        mWasTerminalChunk = true;
        buffer[index] = cEndOfLine;
      }
      else {
        mWasTerminalChunk = false;
      }
      ++i;
      ++index;
    }
    ++mChunkCount[mBufferToWrite];
    if(mWasTerminalChunk) {
      mActiveTaskId = nowtech::cInvalidTaskId;
    }
    else {
      mActiveTaskId = aChunk.getTaskId();
    }
  }
  return *this;
}

void nowtech::TransmitBuffers::transmitIfNeeded() noexcept {
  if(mChunkCount[mBufferToWrite] == 0) {
    return;
  }
  else {
    if(mChunkCount[mBufferToWrite] == mBufferLength) {
      while(mTransmitInProgress.load() == true) {
        tInterface::pause();
      }
      mRefreshNeeded.store(true);
    }
    else { // nothing to do
    }
    if(mTransmitInProgress.load() == false && mRefreshNeeded.load() == true) {
      mTransmitInProgress.store(true);
      tInterface::transmit(mBuffers[mBufferToWrite], mIndex[mBufferToWrite], &mTransmitInProgress);
      mBufferToWrite = 1 - mBufferToWrite;
      mIndex[mBufferToWrite] = 0;
      mChunkCount[mBufferToWrite] = 0;
      mRefreshNeeded.store(false);
      tInterface::startRefreshTimer(&mRefreshNeeded);
    }
    else { // nothing to do
    }
  }
}

