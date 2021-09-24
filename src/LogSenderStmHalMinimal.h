//
// Created by balazs on 2020. 12. 04..
//

#ifndef NOWTECH_LOG_SENDER_STM_HAL_MINIMAL
#define NOWTECH_LOG_SENDER_STM_HAL_MINIMAL

#include "Log.h"
#include "main.h"

namespace nowtech::log {

// For slow transmission medium like UART on embedded this class could implement double buffering.
template<typename tAppInterface, typename tConverter, size_t tTransmitBufferSize>
class SenderStmHalMinimal final {
public:
  using tAppInterface_   = tAppInterface;
  using tConverter_      = tConverter;
  using ConversionResult = typename tConverter::ConversionResult;
  using Iterator         = typename tConverter::Iterator;

  static constexpr bool csVoid = false;

private:
  inline static UART_HandleTypeDef *sSerialDescriptor = nullptr;
  inline static ConversionResult   *sTransmitBuffer;
  inline static Iterator            sBegin;
  inline static Iterator            sEnd;

  SenderStmHalMinimal() = delete;

public:
  static void init(UART_HandleTypeDef * const aUart, uint8_t *aBuffer) {
    sSerialDescriptor = aUart;
    sTransmitBuffer = reinterpret_cast<ConversionResult*>(aBuffer);
    sBegin = sTransmitBuffer;
    sEnd = sTransmitBuffer + tTransmitBufferSize;
  }

  static void done() noexcept {
  }

  static void send(char const * const aBegin, char const * const aEnd) {
    if(sSerialDescriptor != nullptr) {
      HAL_UART_Transmit_DMA(sSerialDescriptor, reinterpret_cast<uint8_t*>(const_cast<char*>(aBegin)), aEnd - aBegin);
      static constexpr uint32_t cTimeout = 10000U; //1s
      uint32_t timer = 0U;
      while(((HAL_UART_GetState(sSerialDescriptor) == HAL_UART_STATE_BUSY_TX) ||
    		  (HAL_UART_GetState(sSerialDescriptor) == HAL_UART_STATE_BUSY_TX_RX)) && (timer < cTimeout)) {
        //TODO implement common task delay function
        //vTaskDelay(1U);
        tx_thread_sleep(1U);
          timer++;
      }
    }
    else { // nothing to do
    }
  }

  static auto getBuffer() {
    return std::pair(sBegin, sEnd);
  }
};
  
}

#endif
