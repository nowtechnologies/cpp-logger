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

#ifndef NOWTECH_LOGUTIL_INCLUDED
#define NOWTECH_LOGUTIL_INCLUDED

#include "Log.h"
#include <atomic>

namespace nowtech {

  /// Auxiliary class, not part of the Log API.
  class CircularBuffer final : public BanCopyMove {
  private:
    LogOsInterface &tInterface;

    /// Counted in chunks
    LogSizeType const mBufferLength;
    LogSizeType const mChunkSize;
    char * const mBuffer;
    Chunk mStuffStart;
    Chunk mStuffEnd;
    LogSizeType mCount = 0;
    LogSizeType mInspectedCount = 0;
    bool mInspected = true;
    Chunk mFound;

  public:
    CircularBuffer(LogOsInterface &aOsInterface, LogSizeType const aBufferLength, LogSizeType const aChunkSize) noexcept
      : tInterface(aOsInterface)
      , mBufferLength(aBufferLength)
      , mChunkSize(aChunkSize)
      , mBuffer(new char[aBufferLength * aChunkSize])
      , mStuffStart(&aOsInterface, mBuffer, aBufferLength, cInvalidTaskId)
      , mStuffEnd(&aOsInterface, mBuffer, aBufferLength, cInvalidTaskId)
      , mFound(&aOsInterface, mBuffer, aBufferLength, cInvalidTaskId) {
    }

    /// Not intended to be destroyed
    ~CircularBuffer() {
      delete[] mBuffer;
    }

    bool isEmpty() const noexcept {
      return mCount == 0;
    }

    bool isFull() const noexcept {
      return mCount == mBufferLength;
    }

    bool isInspected() const noexcept {
      return mInspected;
    }

    void clearInspected() noexcept {
      mInspected = false;
      mInspectedCount = 0;
      mFound = mStuffStart.getData();
    }

    Chunk const &fetch() noexcept {
      mStuffEnd.pop();
      return mStuffEnd;
    }

    Chunk const &peek() const noexcept {
      return mStuffStart;
    }

    void pop() noexcept {
      --mCount;
      ++mStuffStart;
      mFound = mStuffStart.getData();
    }

    void keepFetched() noexcept {
      ++mCount;
      ++mStuffEnd;
    }

    ///  searches for aTaskId in mFound and sets mInspected if none found. @return mFound
    Chunk const &inspect(TaskIdType const aTaskId) noexcept;

    /// Invalidates found element, which will be later eliminated
    void removeFound() noexcept {
      mFound.invalidate();
    }
  };

  /// Auxiliary class, not part of the Log API.
  class TransmitBuffers final : public BanCopyMove {
  private:
    LogOsInterface &tInterface;

    /// counted in chunks
    LogSizeType const mBufferLength;
    LogSizeType const mChunkSize;
    LogSizeType mBufferToWrite = 0;
    char * mBuffers[2];
    LogSizeType mChunkCount[2] = {
      0,0
    };
    LogSizeType mIndex[2] = {
      0,0
    };
    uint8_t mActiveTaskId = cInvalidTaskId;
    bool mWasTerminalChunk = false;
    std::atomic<bool> mTransmitInProgress;
    std::atomic<bool> mRefreshNeeded;

  public:
    TransmitBuffers(LogOsInterface &aOsInterface, LogSizeType const aBufferLength, LogSizeType const aChunkSize) noexcept
      : tInterface(aOsInterface)
      , mBufferLength(aBufferLength)
      , mChunkSize(aChunkSize) {
      mBuffers[0] = new char[aBufferLength * (aChunkSize - 1)];
      mBuffers[1] = new char[aBufferLength * (aChunkSize - 1)];
      mTransmitInProgress.store(false);
      mRefreshNeeded.store(false);
      tInterface::startRefreshTimer(&mRefreshNeeded);
    }

    ~TransmitBuffers() noexcept {
      delete[] mBuffers[0];
      delete[] mBuffers[1];
    }

    bool hasActiveTask() const noexcept {
      return mActiveTaskId != cInvalidTaskId;
    }

    TaskIdType getActiveTaskId() const noexcept {
      return mActiveTaskId;
    }

    bool gotTerminalChunk() const noexcept {
      return mWasTerminalChunk;
    }

    /// Assumes that the buffer to write has space for it
    TransmitBuffers &operator<<(Chunk const &aChunk) noexcept;

    void transmitIfNeeded() noexcept;
  };

} // namespace nowtech

#endif // NOWTECH_LOGUTIL_INCLUDED
