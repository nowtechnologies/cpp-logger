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

// TODO supplement .cpp for FreeRTOS
void nowtech::Log::transmitterThreadFunction() noexcept {
  // we assume all the buffers are valid
  CircularBuffer circularBuffer(tInterface, sConfig->circularBufferLength, mChunkSize);
  TransmitBuffers transmitBuffers(tInterface, sConfig->transmitBufferLength, mChunkSize);
  while(mKeepRunning.load()) {
    // At this point the transmitBuffers must have free space for a chunk
    if(!transmitBuffers.hasActiveTask()) {
      if(circularBuffer.isEmpty()) {
        static_cast<void>(transmitBuffers << circularBuffer.fetch());
      }
      else { // the circularbuffer may be full or not
        static_cast<void>(transmitBuffers << circularBuffer.peek());
        circularBuffer.pop();
      }
    }
    else { // There is a task in the transmitBuffers to be continued
      if(circularBuffer.isEmpty()) {
        Chunk const &chunk = circularBuffer.fetch();
        if(chunk.getTaskId() != nowtech::cInvalidTaskId) {
          if(transmitBuffers.getActiveTaskId() == chunk.getTaskId()) {
            transmitBuffers << chunk;
          }
          else {
            circularBuffer.keepFetched();
          }
        }
        else { // nothing to do
        }
      }
      else if(!circularBuffer.isFull()) {
        if(circularBuffer.isInspected()) {
          Chunk const &chunk = circularBuffer.fetch();
          if(chunk.getTaskId() != nowtech::cInvalidTaskId) {
            if(transmitBuffers.getActiveTaskId() == chunk.getTaskId()) {
              transmitBuffers << chunk;
            }
            else {
              circularBuffer.keepFetched();
            }
          }
          else { // nothing to do
          }
        }
        else {
          Chunk const &chunk = circularBuffer.inspect(transmitBuffers.getActiveTaskId());
          if(!circularBuffer.isInspected()) {
            transmitBuffers << chunk;
            circularBuffer.removeFound();
          }
          else { // nothing to do
          }
        }
      }
      else { // the circular buffer is full
        static_cast<void>(transmitBuffers << circularBuffer.peek());
        circularBuffer.pop();
        circularBuffer.clearInspected();
      }
    }
    if(transmitBuffers.gotTerminalChunk()) {
      circularBuffer.clearInspected();
    }
    else {
    }
    transmitBuffers.transmitIfNeeded();
  }
}


















