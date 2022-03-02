#define PICOBENCH_IMPLEMENT
#define PICOBENCH_DEFAULT_SAMPLES 10
#include "picobench/picobench.hpp"

#include "LogAppInterfaceStd.h"
#include "LogConverterCustomText.h"
#include "LogSenderStdOstream.h"
#include "LogQueueStdCircular.h"
#include "LogQueueVoid.h"
#include "LogMessageCompact.h"
#include "LogMessageVariant.h"
#include "Log.h"
#include <fstream>
#include <memory>

constexpr nowtech::log::TaskId cgMaxTaskCount = 1;
constexpr bool cgAllowRegistrationLog = true;
constexpr bool cgLogFromIsr = false;
constexpr size_t cgTaskShutdownSleepPeriod = 100u;
constexpr bool cgArchitecture64 = true;
constexpr uint8_t cgAppendStackBufferSize = 100u;
constexpr bool cgAppendBasePrefix = true;
constexpr bool cgAlignSigned = false;
using AtomicBufferType = int32_t;
constexpr size_t cgAtomicBufferExponent = 14u;
constexpr AtomicBufferType cgAtomicBufferInvalidValue = 1234546789;
constexpr size_t cgTransmitBufferSize = 444444u;
constexpr size_t cgPayloadSize = 8u;
constexpr bool cgSupportFloatingPoint = true;
constexpr size_t cgQueueSize = 444444u;
constexpr nowtech::log::LogTopic cgMaxTopicCount = 2;
constexpr nowtech::log::TaskRepresentation cgTaskRepresentation = nowtech::log::TaskRepresentation::cName;
constexpr size_t cgDirectBufferSize = 0u;
constexpr nowtech::log::ErrorLevel cgErrorLevel = nowtech::log::ErrorLevel::Info;

using LogAppInterface = nowtech::log::AppInterfaceStd<cgMaxTaskCount, cgLogFromIsr, cgTaskShutdownSleepPeriod>;
constexpr typename LogAppInterface::LogTime cgTimeout = 123u;
constexpr typename LogAppInterface::LogTime cgRefreshPeriod = 444;
using LogMessage = nowtech::log::MessageCompact<cgPayloadSize, cgSupportFloatingPoint>;
using LogConverter = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderStdOstream<LogAppInterface, LogConverter, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueStdCircular<LogMessage, LogAppInterface, cgQueueSize>;
using LogAtomicBuffer = nowtech::log::AtomicBufferOperational<LogAppInterface, AtomicBufferType, cgAtomicBufferExponent, cgAtomicBufferInvalidValue>;
using LogConfig = nowtech::log::Config<cgAllowRegistrationLog, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod, cgErrorLevel>;
using Log = nowtech::log::Log<LogQueue, LogSender, LogAtomicBuffer, LogConfig>;

nowtech::log::TaskId gTaskId;

void nowtechLogRefTaskId(picobench::state& s) {
  for (auto _ : s) {
    Log::i<Log::info>(gTaskId) << "Info message" << Log::end;
  }
}
PICOBENCH(nowtechLogRefTaskId);

void nowtechLogCopyTaskId(picobench::state& s) {
  for (auto _ : s) {
    Log::i<Log::info>(gTaskId) << LC::St << "Info message" << Log::end;
  }
}
PICOBENCH(nowtechLogCopyTaskId);

void nowtechLogRefNoTaskId(picobench::state& s) {
  for (auto _ : s) {
    Log::i<Log::info>() << "Info message" << Log::end;
  }
}
PICOBENCH(nowtechLogRefNoTaskId);

void nowtechLogCopyNoTaskId(picobench::state& s) {
  for (auto _ : s) {
    Log::i<Log::info>() << LC::St << "Info message" << Log::end;
  }
}
PICOBENCH(nowtechLogCopyNoTaskId);


int main(int argc, char* argv[])
{
  nowtech::log::LogFormatConfig logConfig;
  std::ofstream out("tmp");
  LogSender::init(&out);
  Log::init(logConfig);
  Log::registerCurrentTask("main");
  gTaskId = Log::getCurrentTaskId();

  picobench::runner r;
  r.parse_cmd_line(argc, argv);
  auto ret = r.run();
  Log::unregisterCurrentTask();
  Log::done();
  return ret;
}
