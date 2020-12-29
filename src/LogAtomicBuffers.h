#ifndef NOWTECH_LOG_ATOMIC_BUFFERS
#define NOWTECH_LOG_ATOMIC_BUFFERS

#include<atomic>
#include<utility>
#include<algorithm>

namespace nowtech::log {

template<typename tAppInterface, typename tAtomicBufferType, std::size_t tAtomicBufferSizeExponent, tAtomicBufferType tInvalidValue>
class AtomicBufferOperational final {
public:
  using tAtomicBufferType_ = tAtomicBufferType;
  static constexpr std::size_t csAtomicBufferSizeExponent = tAtomicBufferSizeExponent;
  static constexpr std::size_t csAtomicBufferSize = 1u << tAtomicBufferSizeExponent;
  static constexpr tAtomicBufferType csInvalidValue = tInvalidValue;

private:
  inline static tAtomicBufferType       *sBuffer;  // We spare one instruction per access by having C-style array instead of std::array
  inline static std::atomic<std::size_t> sNextWrite;
  inline static std::atomic<bool>        sShouldSend;

public:
  static void init() {
    sNextWrite = 0u;
    sShouldSend = false;
    sBuffer = tAppInterface::template _newArray<tAtomicBufferType>(csAtomicBufferSize);
    invalidate();
  }

  static void done() {
    tAppInterface::template _deleteArray<tAtomicBufferType>(sBuffer);
  }

  static void push(tAtomicBufferType const aValue) noexcept {
    std::size_t nextIndex = sNextWrite++ % csAtomicBufferSize;
    sBuffer[nextIndex] = aValue;
  }

  static void scheduleForSend() noexcept {
    sShouldSend = true;
  }

  static bool isScheduledForSent() noexcept {
    return sShouldSend.load();
  }

  static void sendFinished() noexcept {
    sShouldSend = false;
  }

  static auto getBuffer() noexcept {
    return std::pair<tAtomicBufferType const*, size_t>{sBuffer, sNextWrite % csAtomicBufferSize};
  }

  static void invalidate() noexcept {
    std::fill_n(sBuffer, csAtomicBufferSize, tInvalidValue);
  }
};

class AtomicBufferVoid final {
public:
  using tAtomicBufferType_ = char;
  static constexpr std::size_t csAtomicBufferSizeExponent = 0u;
  static constexpr std::size_t csAtomicBufferSize = 1u;

  static void init() { // nothing to do
  }

  static void done() { // nothing to do
  }

  static void push(tAtomicBufferType_ const) noexcept { // nothing to do
  }

  static void scheduleForSend() noexcept { // nothing to do
  }

  static bool isScheduledForSent() noexcept { // nothing to do
    return false;
  }

  static void sendFinished() noexcept { // nothing to do
  }

  static std::pair<tAtomicBufferType_ const*, size_t> getBuffer() noexcept { // nothing to do
    return std::pair(nullptr, 0u);
  }

  static void invalidate() noexcept { // nothing to do
  }
};

}

#endif
