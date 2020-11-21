#ifndef LOG_QUEUE_VOID
#define LOG_QUEUE_VOID

#include <cstddef>

namespace nowtech::log {

template<typename tMessage, size_t tQueueSize>
class QueueVoid final {
public:
  using tMessage_ = tMessage;

private:
  QueueVoid() = delete;

public:
  static void init() { // nothing to do
  }

  static void done() {  // nothing to do
  }
};

}

#endif