# C++ template-based crossplatform log library

```C++
// Stripped template instantiations.
nowtech::log::LogConfig logConfig;
LogSender::init(&std::cout);
Log::init(logConfig);
Log::registerTopic(nowtech::LogTopics::system, "system");
Log::registerCurrentTask("main");

//...

Log::i(nowtech::LogTopics::system) << "bulk data size:" << someCollection.size() << Log::end;  // one group of 2 items

auto logger = Log::n() << "bulk data follows:"; // one group of many items starts
for(auto item : someCollection) {
  logger << LC::X4 << item;                     // format: hexadecimal, fill to 4 digits
}
logger << Log::end;                             // the group of many items ends
```

This is a complete rework of my [old logger](https://github.com/balazs-bamer/cpp-logger/tree/old/) after identifying the design flaws and performance problems. I used C++20 -capable compilers during development, but the code is probably C++17 compatible.

The code was written to possibly conform MISRA C++ and High-Integrity C++.

The library is published under the [MIT license](https://opensource.org/licenses/MIT).

Copyright 2020 Balázs Bámer.

## Aims

- Provide a common logger API for cross-platform embedded / desktop code.
- Use as minimal code and data space as possible. I'm not sure if I'm was good enough obtaining it. On STM32 Cortex M3, flash overhead of direct sending mode is **4k**, queued multithreading mode id **6k**. Anyway, no more time to further develop this library.
- The library was designed to allow the programmer control log bandwidth usage, program and data memory consumption.
- Log arithmetic C++ types and strings with or without copying them (beneficial for literals).
  - String literals or string constants can be transferred using their pointers to save MCU cycles.
  - More transient strings will be copied instantly.
- Operate in a _strictly typed_ way as opposed to the printf-based loggers some companies still use today.
- We needed a much finer granularity than the usual log levels, so I've introduced independently selectable topics. However, these allow easy simulation of traditional log levels, see in the API section.
- Important aim was to let the user log a _group of related items_ without other tasks interleaving the output of converting these items.
- Possibly minimal relying on external libraries. It does not depend on numeric to string conversion functions.
- In contrast with the original solution, the new one extensively uses templated classes to
  - implement a flexible plugin architecture for adapting different user needs
  - let the compiler optimizer do its finest.
  - _totally eliminate_ logging code generation with at least -O1 optimization level with a one-line change (template logic, without #ifdef).
- Let the user decide to rely on C++ exceptions or implement a different kind of error management.
- Let me learn about recent language features like C++20 concepts. Well, I found out that these are not designed for specifying Java-like interfaces for templated classes. At least I couldn't formulate them.

## Architecture

The logger consists of six classes:
- `Log` - the main class providing API and base architecture. This accepts the other ones as template parameters.
- _Queue_ - used to transfer the converting and sending operations from the current task to a background one.
- _Converter_ - converts the user types to strings or any other binary format to be sent or stored.
- _Sender_ - the class responsible for transmitting or storing the converted data.
- _App interface_ - used to interface the application, STL and OS. This provides
  - Possibly custom memory management needed by embedded applications.
  - Task management, including
    - Obtaining byte task IDs required by the Log class.
    - Registering and unregistering the current task.
    - Obtaining the task name in C string.
  - Obtaining timestamps using some OS / STL time routine.
  - Error management.
- _Message_ - used as a transfer medium in the Queue. For numeric types and strings transferred by pointers, one message is needed per item. For stored strings several messages may be necessary. There are two general-purpose implementations:
  - a variant based message.
  - a more space-efficient handcrafted message.

The logger can operate in two modes:
- Direct without queue, when the conversion and sending happens without message instantiation and queue usage. This can be useful for single-threaded applications.
- With queue, when each item sent will form one or more (in case of stored strings) message and use a central thread-safe queue. On the other end of the queue a background thread pops the messages and groups them by tasks in secondary lists. When a group of related items from a task has arrived, its conversion and sending begins.

## Implementations

The current library provides some simple implementations for STL and Boost based desktop and server applications and FreeRTOS based embedded applications. The whole implementation assumes an architecture of at least 32 bits.

### Log

This is a general implementation, but tailored for embedded requirements.

In queue-less mode, conversion and sending happens immediately for each item. Thus it is desirable that the Sender has some sort of buffering inside.

For the queue mode, it contains a secondary list or queue for each task, which gather the items in each logged group. After the terminal item arrives, conversion happens for each item in the sender buffer and then comes the sending. These secondary queues are backed by a pool allocator to avoid repeated dynamic memory access.

### ConverterCustomText

A simple converter emitting character strings, with an emphasis on space-efficient operation on embedded platforms. Features:
- Arbitrary numeric base from 2 to 16 for integer conversion.
- Floating-point conversion always happens in scientific mode with NAN and INF distinction (bug: no signs displayed for them).
- Adjustable
  - zero fill
  - numeric base prefix display for binary and hexadecimal
  - extra space before positive numbers to be aligned with negatives
- Automatically adds space between items of a group.

### AppInterfaceFreeRtosMinimal

This implementation assumes FreeRTOS 10.0.1, but should work as well as with 9.x or perhaps even older. The main objective was to keep it as simple and small as possible. It provides global overload of new and delete operators using FreeRTOS' dynamic memory management, but itself uses only a statically allocated array. It uses a linear array for task registry and omits unregistering, because a typical embedded application creates all the tasks beforehand and never kills them. Task names are native FreeRTOS task names. For similar reasons, logger shutdown is not implemented.
There is a design flaw in the library: checking for ISRs happen here, so the FreeRTOS implementation contains an MCU-specific function, here for STM32. However, it is easy to replace with the one for the actual MCU.

### AppInterfaceStd

This is a general desktop-oriented C++17 STL implementation targeting speed over space. It uses a hash set and thread local storage for task registration, and task unregistration is also supported. The task registy API is protected by a mutex. Other, more frequently called functions work without locking. Logger initialization and shutdown are properly implemented.

### QueueVoid

Empty implementation, used for placeholder when no queue is needed.

### QueueFreeRtos

It uses the FreeRTOS' built-in queue, so no locking is needed on sending. Sending into the queue can happen from an ISR.

### QueueStdBoost

This one uses a multi-producer multi-consumer lockfree queue of Boost, so no locking is needed either.

### SenderVoid

Emply implementation for the case when all the log calls have to be eliminated from the binary. This happens at gcc and clang optimization levels -Os, -O1, -O2 and -O3. The application can use a template metaprogramming technique to declare a Log using this as the appropriate parameter, so no #ifdef is needed.

### SenderStmHalMinimal

This is a minimal implementation for STM32 UART using blocking HAL transmission. A more sophisticated one would use double buffering with DMA to use the MCU more efficiently and allow buffering new data during the potentially slow transmission of the old buffer. This mechanism is implemented in the old version.

### LogSenderStdOstream

It is a simple std::ostream wrapper.

## Space requirements

I have investigated several scenarios using simple applications which contain practically nothing but a task creation apart of the logging. This way I could measure the net space required by the log library and its necessary supplementary functions like std::unordered_set or floating-point emulation for log10.

For desktop, I used clang version 10.0.0 on x64. For embedded, I used arm-none-eabi-g++ 10.1.1 on STM32 Cortex M3. This MCU needs emulation for floating point. All measurements were conducted using -Os. I present the net growths in segments text, data and BSS for each of the following scenarios:
- direct logging (for single threaded applications)
- logging turned off with SenderVoid
- logging with queue using MessageVariant (for multithreaded applications)
- logging with queue using MessageCompact (for multithreaded applications)

### FreeRTOS with floating point

To obtain net results, I put some floating-point operations in the application *test-sizes-freertosminimal-float.cpp* because a real application would use them apart of logging. 

|Scenario      |   Text|  Data|    BSS|
|--------------|------:|-----:|------:|
|direct        |13152  | 108  |52     |
|off           |0      |0     |0      |
|MessageVariant|15304  |112   | 76    |
|MessageCompact|15024  |112   | 76    |

### FreeRTOS without floating point

No floating point arithmetics in the application and the support is turned off in the logger. Source is *test-sizes-freertosminimal-nofloat.cpp*

|Scenario      |   Text|  Data|    BSS|
|--------------|------:|-----:|------:|
|direct        |4303   | 8    |56     |
|off           |0      |0     |0      |
|MessageVariant|6440   |12    | 80    |
|MessageCompact|6192   |12    | 80    |

### x86 STL with floating point

Not much point to calculate size growth here, but why not? Source is *test-sizes-stdthreadostream.cpp*

|Scenario      |   Text|  Data|    BSS|
|--------------|------:|-----:|------:|
|direct        |11899  | 273  |492    |
|off           |0      |0     |0      |
|MessageVariant|22483  | 457  |892    |
|MessageCompact|20851  |457   | 892   |

## API

### Supported types

As all major coding standards suggest, use of integer types with indeterminate size is discouraged, so this library does not support `long` or `unsigned`.

`LogFormat` is a struct holding the numeric base and the fill value. It is easiest to reach in `LogConfig`.

|C++ type       |LogFormat prefix affects it|Remark                           |
|---------------|---------------------------|---------------------------------|
|`bool`         |no                         |Appears as _true_ or _false_ in the current `LogConverterCustomText` implementation.|
|`float`        |mantissa precision         |Enabled only if floating point support is on.|
|`double`       |mantissa precision         |Enabled only if floating point support is on and the payload is chosen to be big enough.|
|`long double`  |mantissa precision         |Enabled only if floating point support is on and the payload is chosen to be big enough.|
|`uint8_t`      |base and fill              ||
|`uint16_t`     |base and fill              ||
|`uint32_t`     |base and fill              ||
|`uint64_t`     |base and fill              ||
|`int8_t`       |base and fill              ||
|`int16_t`      |base and fill              ||
|`int32_t`      |base and fill              ||
|`int64_t`      |base and fill              ||
|`char`         |no                         |This and the strings support only plain old 8-bit ASCII.|
|`char const *` |for no prefix              |Can be of arbitrary length for string constants.|
|`char const *` |for `LC::St` prefix        |Only a limited length of _payload size_ * 255 characters can be transferred from a transient string.|

64-bit integer types require emulation on 32-bit architectures. By default, `LogConverterCustomText`'s templated conversion routine use 32-bit numbers to gain speed. Using 64-bit operands instantiates the 64-bit emulated routines as well, which takes extra flash space on embedded.

### Initialization

Log system initialiation consists of the following steps:
1. Declare possible topics (see next section).
2. Instantiate templates.
3. Define and fill `LogConfig` struct. All its fields have default value.
4. Initialize the _Sender_. This has implementation-specific arguments for the actual output.
5. Initialize the _Log_ using the _LogConfig_ instance.
6. Register the required topics (see nextr section).
7. Call `Log::registerCurrentTask("task name");` for  each interested task. Other tasks won't be able to log.

Refer the beginning for an example for STL without the first step. Here is a FreeRTOS template declaration without floating point support but for multithreaded mode:

```C++
constexpr nowtech::log::TaskId cgMaxTaskCount = 1u;
constexpr bool cgLogFromIsr = false;
constexpr size_t cgTaskShutdownPollPeriod = 100u;
constexpr bool cgArchitecture64 = false;
constexpr uint8_t cgAppendStackBufferSize = 100u;
constexpr bool cgAppendBasePrefix = true;
constexpr bool cgAlignSigned = false;
constexpr size_t cgTransmitBufferSize = 123u;
constexpr size_t cgPayloadSize = 6u;            // This disables 64-bit integer arithmetic.
constexpr bool cgSupportFloatingPoint = false;
constexpr size_t cgQueueSize = 111u;
constexpr nowtech::log::LogTopic cgMaxTopicCount = 2;
constexpr nowtech::log::TaskRepresentation cgTaskRepresentation = nowtech::log::TaskRepresentation::cName;
constexpr uint32_t cgLogTaskStackSize = 256u;
constexpr uint32_t cgLogTaskPriority = tskIDLE_PRIORITY + 1u;

constexpr size_t cgDirectBufferSize = 0u;
using LogAppInterfaceFreeRtosMinimal = nowtech::log::AppInterfaceFreeRtosMinimal<cgMaxTaskCount, cgLogFromIsr, cgTaskShutdownPollPeriod>;
constexpr typename LogAppInterfaceFreeRtosMinimal::LogTime cgTimeout = 123u;
constexpr typename LogAppInterfaceFreeRtosMinimal::LogTime cgRefreshPeriod = 444;
using LogMessage = nowtech::log::MessageCompact<cgPayloadSize, cgSupportFloatingPoint>;
using LogConverterCustomText = nowtech::log::ConverterCustomText<LogMessage, cgArchitecture64, cgAppendStackBufferSize, cgAppendBasePrefix, cgAlignSigned>;
using LogSender = nowtech::log::SenderStmHalMinimal<LogAppInterfaceFreeRtosMinimal, LogConverterCustomText, cgTransmitBufferSize, cgTimeout>;
using LogQueue = nowtech::log::QueueFreeRtos<LogMessage, LogAppInterfaceFreeRtosMinimal, cgQueueSize>;
using Log = nowtech::log::Log<LogQueue, LogSender, cgMaxTopicCount, cgTaskRepresentation, cgDirectBufferSize, cgRefreshPeriod>;
```

Explanation of configuration parameters:
|Name in the library source                                |Goes in                  |Remark             |
|----------------------------------------------------------|-------------------------|-------------------|
|`TaskId tMaxTaskCount`                                    |_App interface_          |TaskId is `uint8_t`. Maximum value is 254.|
|`bool tLogFromIsr`                                        |_App interface_          |Determines if logging from ISR is enabled (when applicable).|
|`size_t tTaskShutdownPollPeriod`                          |_App interface_          |Polling interval in implementation-defined unit (usually ms) for log system shutdown.|
|`size_t tPayloadSize`                                     |_Message_                |Maximum size of payload in bytes.|
|`bool tSupportFloatingPoint`                              |_Message_                |Floating point support.|
|`typename tMessage`                                       |_Converter_              |The _Message_ type to use.|
|`bool tArchitecture64`                                    |_Converter_              |Tells if we are on 64-bit architecture (if not, the 32-bit). Well, it could have been figured out.|
|`uint8_t tAppendStackBufferSize`                          |_Converter_              |Size of stack buffer used for binary to text conversion.|
|`bool tAppendBasePrefix`                                  |_Converter_              |If true base-2 or base-16 conversion should prepend _0b_ or _0x_.|
|`bool tAlignSigned`                                       |_Converter_              |If true, positive numbers will get an extra ' ' to be aligned with negatives. |
|`typename tAppInterface`                                  |_Sender_                 |The _app interface_ type to use.|
|`typename tConverter`                                     |_Sender_                 |The _Converter_ type to use.|
|`size_t tTransmitBufferSize`                              |_Sender_                 |Length of buffer to use for conversion. This should be sufficient for the joint size of possible items in the largest group.|
|`typename tAppInterface::LogTime tTimeout`                |_Sender_                 |Timeout in implementation-defined unit (usually ms) for transmission.|
|`typename tMessage`                                       |_Queue_                  |The _Message_ type to use.|
|`typename tAppInterface`                                  |_Queue_                  |The _app interface_ type to use.|
|`size_t tQueueSize`                                       |_Queue_                  |Number of items the queue should hold. This applies to the master queue and to the aggregated capacity of the per-task queues.|
|`typename tQueue`                                         |`Log`                    |The _Queue_ type to use.|
|`typename tSender`                                        |`Log`                    |The _Sender_ type to use.|
|`LogTopic tMaxTopicCount`                                 |`Log`                    |LogTopic is `int8_t`. Maximum is 127.|
|`TaskRepresentation tTaskRepresentation`                  |`Log`                    |One of `cNone` (for omitting it), `cId` (for numeric task ID), `cName` (for task name).|
|`size_t tDirectBufferSize`                                |`Log`                    |When 0, the given _Queue_ will be used. Otherwise, it is the size of a buffer on stack to hold a converted item before sending it.|
|`typename tSender::tAppInterface_::LogTime tRefreshPeriod`|`Log`                    |Timeout in implementation-defined unit (usually ms) for waiting on the queue before sending what already present.|
|`bool allowRegistrationLog`                               |`LogConfig`              |True if task (un)registering should be logged.|
|`LogFormat taskIdFormat`                                  |`LogConfig`              |Format of task ID to use when `tTaskRepresentation == TaskRepresentation::cId`.|
|`LogFormat tickFormat`                                    |`LogConfig`              |Format for displaying the timestamp in the header, if any. Should be `LogConfig::cInvalid` to disable tick output.|
|`LogFormat defaultFormat`                                 |`LogConfig`              |Default formatting, initially `LogConfig::Fm` to obtain maximum possible precision for floating point types.|

### Topics and log levels

The log system supports individually selectable topics to log. I've decided so because we needed a much finer granularity than the tradiotional log levels. These topics have to be declared first, after which they still would be disabled.

```C++
namespace nowtech::LogTopics {
nowtech::log::TopicInstance level1;
nowtech::log::TopicInstance level2;
nowtech::log::TopicInstance level3;
nowtech::log::TopicInstance someTopic;
nowtech::log::TopicInstance someOtherTopic;
}
```

To enable some of them, the interesting ones must be registered in the log system. Although log levels are not supported natively, here is a workaround to define them.

```C++
#ifdef LEVEL1
Log::registerTopic(nowtech::LogTopics::level1, "level1");
#elif LEVEL2
Log::registerTopic(nowtech::LogTopics::level1, "level1");
Log::registerTopic(nowtech::LogTopics::level2, "level2");
#elif LEVEL3
Log::registerTopic(nowtech::LogTopics::level1, "level1");
Log::registerTopic(nowtech::LogTopics::level2, "level2");
Log::registerTopic(nowtech::LogTopics::level3, "level3");
#endif
```

### Logging

All the logging API is implemented as static functions in the `Log` template class. Logging happens using a `std::ostream` -like API, like in the example in the beginning. There are two overloaded functions to start the chain:
- `static LogShiftChainHelper i(...) noexcept` writes any header configured for the application.
- `static LogShiftChainHelper n(...) noexcept` omits this header, just writes the actual stuff it receives using `<<`.
Note, LogShiftChainHelper implementation depends on the given log mode (direct / queued / shut off).

Each function has four overloads with the following parameter signatures:
- `()` - logs unconditionally, and queries the task ID.
- `(TaskId const aTaskId)` - logs unconditionally using the supplied task ID.
- `(LogTopic const aTopic)` - logs depending on the given task ID was registered, and queries the task ID.
- `(LogTopic const aTopic, TaskId const aTaskId)` - logs depending on the given task ID was registered using the supplied task ID.

One can use the `static TaskId getCurrentTaskId() noexcept` function to query the current task ID and store it, This can be important if querying the task ID is expensive on the given platform.

If you have many or unknown number of items to log, you can use the form

```C++
auto logger = Log::n() << "someCollection contgents:";
for(auto item : someCollection) {
  logger << item;
}
logger << Log::end;  // the group of many items ends
```

I've implemented a function call-like entry point using C++17 folding expressions. To be honest, this is just a _why not_ solution, and not an integral part of the API. It gets called like

```C++
Log::f(Log::i(nowtech::LogTopics::someTopic), some, variables, follow);
```

and apart of being clumsy, it is even takes more binary space than the `std::ostream` -like API it uses under the hood. It appends `Log::end` automatically.
