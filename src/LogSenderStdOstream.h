#ifndef NOWTECH_LOG_SENDER_STD_OSTREAM
#define NOWTECH_LOG_SENDER_STD_OSTREAM

#include <ostream>
#include "Log.h"

namespace nowtech::log {

template<typename tAppInterface, typename tConverter, size_t tTransmitBufferSize, typename tAppInterface::LogTime tTimeout, typename tAppInterface::LogTime tRefreshPeriod>
class SenderStdOstream final {
public:
  using tAppInterface_ = tAppInterface;
  using tConverter_ = tConverter;

private:
  inline static std::ostream *sStream = nullptr;

  SenderStdOstream() = delete;

public:
  static void init(std::ostream * const aStream) noexcept {
    sStream = aStream;
  }

  static void send(char const * const aBegin, char const * const aEnd) {
    try {
      if(sStream != nullptr) {
        sStream->write(aBegin, aEnd - aBegin);
      }
      else { // nothing to do
      }
    } catch(...) {
      tAppInterface::error(Exception::cSenderError);
    }
  }
};
  
}

#endif