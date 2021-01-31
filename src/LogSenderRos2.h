#ifndef NOWTECH_LOG_SENDER_ROS2
#define NOWTECH_LOG_SENDER_ROS2

#include <string>
#include "rcutils/logging.h"
#include "Log.h"

namespace nowtech::log {

// The buffer it gets in iterators MUST NOT contain %. Otherwise undefined behavior occurs.
template<typename tAppInterface, typename tConverter, size_t tTransmitBufferSize, typename tAppInterface::LogTime tTimeout>
class SenderRos2 final {
public:
  using tAppInterface_   = tAppInterface;
  using tConverter_      = tConverter;
  using ConversionResult = typename tConverter::ConversionResult;
  using Iterator         = typename tConverter::Iterator;

  static constexpr bool csVoid = false;

private:
  inline static ConversionResult *sTransmitBuffer;
  inline static Iterator          sBegin;
  inline static Iterator          sEnd;
  static constexpr int            csDummySeverity       = 50;  // TODO value found by trial-error
  inline static constexpr char    csDummyName[]         = "";
  inline static constexpr char    csDummyFunctionName[] = "";
  inline static constexpr char    csDummyFilename[]     = "";
  inline static constexpr rcutils_log_location_t csDummyLocation = { csDummyFunctionName, csDummyFilename, 0u };

  SenderRos2() = delete;

public:
  static void init() {
    sTransmitBuffer = tAppInterface::template _newArray<ConversionResult>(tTransmitBufferSize);
    sBegin = sTransmitBuffer;
    sEnd = sTransmitBuffer + tTransmitBufferSize;
  }

  static void done() noexcept {
    tAppInterface::template _deleteArray<ConversionResult>(sTransmitBuffer);
  }

  static void send(char const * const aBegin, char const * const aEnd) {
    try {
      std::string buffer{aBegin, static_cast<size_t>(aEnd - aBegin)};
      rcutils_log(&csDummyLocation, csDummySeverity, csDummyName, buffer.c_str());
    } catch(...) {
      tAppInterface::error(Exception::cSenderError);
    }
  }

  static auto getBuffer() {
    return std::pair(sBegin, sEnd);
  }
};
  
}

#endif