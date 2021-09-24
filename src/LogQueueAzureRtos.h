//
// Created by lantiati on 2021. 09. 16.
//

#ifndef NOWTECH_LOG_QUEUE_AZURERTOS
#define NOWTECH_LOG_QUEUE_AZURERTOS

#include <cstddef>
#include "main.h"
#include "tx_api.h"
#include <cassert>
#include "OnlyAllocate.h"

namespace nowtech::log {

template<typename tMessage, typename tAppInterface, size_t tQueueSize>
class QueueAzureRtos final {
public:
  using tMessage_ = tMessage;
  using tAppInterface_ = tAppInterface;
  using LogTime = typename tAppInterface::LogTime;

  static constexpr size_t csQueueSize = tQueueSize;
private:
  inline static CHAR cLogQueName[] =  "LoggerTxQueue";
  inline static TX_QUEUE sQueue;

  QueueAzureRtos() = delete;
  static constexpr UINT cMessageSize  = sizeof(tMessage)/sizeof(ULONG)+1;
public:
  static void init() { // nothing to do
     void *queueBufferPnt = tOnlyAllocator::_newArray<uint8_t>(csQueueSize * cMessageSize * sizeof(ULONG));
      UINT status = tx_queue_create(
          &sQueue,
          cLogQueName,
          5, //cMessageSize,
          queueBufferPnt,
          csQueueSize);
      if(status != TX_SUCCESS){
        Error_Handler();
      }
  }

  static void done() {  // nothing to do
  }

  static bool empty() noexcept {
    bool retval = false;
    CHAR *name;
    ULONG enqueued;
    ULONG availableStorage;
    TX_THREAD *firstSuspended;
    ULONG suspendedCount;
    TX_QUEUE *nextQueue;
    UINT status;
    // Retrieve information about message queue
    status = tx_queue_info_get(&sQueue, &name,
                               &enqueued, &availableStorage,
                               &firstSuspended, &suspendedCount,
                               &nextQueue);
    //Check if the info get is successful and the que is empty.
    if((status == TX_SUCCESS)
        && (enqueued == 0U)){
      retval = true;
    }
    return retval;
  }

  static void push(tMessage const &aMessage) noexcept {
     // Don't care about the result, pop logic will manage missing messages.
     uint8_t txMsg[cMessageSize*sizeof(ULONG)];
     memcpy(txMsg, reinterpret_cast<const void*>(aMessage.mData), sizeof(aMessage));
     tx_queue_send(&sQueue, &txMsg, TX_NO_WAIT);
  }

  static bool pop(tMessage &aMessage, LogTime const aPauseLength) noexcept {
    uint8_t rxMsg[cMessageSize*sizeof(ULONG)];
    UINT status = TX_QUEUE_ERROR;
    status = tx_queue_receive(&sQueue, &rxMsg, aPauseLength);
    memcpy(reinterpret_cast<void*>(aMessage.mData), rxMsg, sizeof(aMessage));
    return (status == TX_SUCCESS);
  }
};

}

#endif
