#define PICOBENCH_IMPLEMENT
#define PICOBENCH_DEFAULT_SAMPLES 10
#include "picobench/picobench.hpp"

#include "LogAppInterfaceStd.h"
#include "LogConverterCustomText.h"
#include "LogSenderStdOstream.h"
#include "LogSenderVoid.h"
#include "LogQueueStdBoost.h"
#include "LogQueueVoid.h"
#include "LogMessageCompact.h"
#include "LogMessageVariant.h"
#include "Log.h"
#include <fstream>


constexpr nowtech::log::TaskId cgMaxTaskCount = 1;
constexpr bool cgLogFromIsr = false;
constexpr size_t cgTaskShutdownSleepPeriod = 100u;
constexpr bool cgArchitecture64 = true;
constexpr uint8_t cgAppendStackBufferSize = 100u;
constexpr bool cgAppendBasePrefix = true;
constexpr bool cgAlignSigned = false;
constexpr size_t cgTransmitBufferSize = 123u;
constexpr size_t cgPayloadSize = 8u;
constexpr bool cgSupportFloatingPoint = true;
constexpr size_t cgQueueSize = 444u;
constexpr nowtech::log::LogTopic cgMaxTopicCount = 2;
constexpr nowtech::log::TaskRepresentation cgTaskRepresentation = nowtech::log::TaskRepresentation::cName;
constexpr nowtech::log::ErrorLevel cgRequiredLevel = nowtech::log::ErrorLevel::Info;

constexpr size_t cgDirectBufferSize = 1024u;
using LogAppInterfaceStd = nowtech::log::AppInterfaceStd<cgMaxTaskCount, cgLogFromIsr, cgTaskShutdownSleepPeriod>;
constexpr typename LogAppInterfaceStd::LogTime cgTimeout = 123u;
constexpr typename LogAppInterfaceStd::LogTime cgRefreshPeriod = 444;
using LogMessage = nowtech::log::MessageVariant<cgPayloadSize, cgSupportFloatingPoint>;
using LogConverterCustomText = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderStdOstream<LogAppInterfaceStd, LogConverterCustomText, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueVoid<LogMessage, LogAppInterfaceStd, cgQueueSize>;
using Log = nowtech::log::Log<LogQueue, LogSender, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod, cgRequiredLevel>;

nowtech::log::TaskId gTaskId;

void nowtechLogRef(picobench::state& s) {
  for (auto _ : s) {
    Log::i<Log::info>(gTaskId) << "Info message" << Log::end;
  }
}
PICOBENCH(nowtechLogRef);

int main(int argc, char* argv[])
{
  nowtech::log::LogConfig logConfig;
  logConfig.allowRegistrationLog = false;
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
