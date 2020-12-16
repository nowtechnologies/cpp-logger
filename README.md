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

auto logger = Log::n() << "bulk data follows:";  // one group of many items starts
for(auto item : someCollection) {
  logger << item;
}
logger << Log::end;  // the group of many items ends
```

This is a complete rework of my [old logger](https://github.com/balazs-bamer/cpp-logger/tree/old/) after identifying the design flaws and performance problems. I used C++20 -capable compilers during development, but the code is probably C++17 compatible.

The code was written to possibly conform MISRA C++ and High-Integrity C++.

The library is published under the [MIT license](https://opensource.org/licenses/MIT).

Copyright 2020 Balázs Bámer.

## Aims

- Provide a common logger API for cross-platform embedded / desktop code.
- Use as minimal code and data space as possible. I'm not sure if I'm was good enough obtaining it. Anyway, no more time to further develop this library.
- The library was designed to allow the programmer control log bandwidth usage, program and data memory consumption.
- Log arithmetic C++ types and strings with or without copying them (beneficial for literals).
  - String literals or string constants can be transferre using their pointers to save MCU cycles.
  - More transient strings will be copied instantly.
- Operate in a strictly typed way as opposed to the printf-based loggers some companies still use today.
- We needed a much finer granularity than the usual log levels, so I've introduced independently selectable topics.
- Important aim was to let the user log a group of related items without other tasks interleaving the output of converting these items.
- Possibly minimal relying on external libraries. It does not depend on numeric to string conversion functions.
- In contrast with the original solution, the new one extensively uses templated classes to
  - implement a flexible plugin architecture for adapting different user needs
  - let the compiler optimizer do its finest.
  - totally eliminate logging code generation with at least -O1 optimization level with a one-line change (template logic, without #ifdef).
- Let the user decide to rely on C++ exceptions or implement a different kind of error management.
- Let me learn about recent language features like C++20 concepts. Well, I found out that these are not designed for specifying Java-like interfaces for templated classes. At least I couldn't formulate them.

## Architecture

The logger consists of six classes:
- Log - the main class providing API and base architecture. This accepts the other ones as template parameters.
- Queue - used to transfer the converting and sending operations from the current task to a background one.
- Converter - converts the user types to strings or any other binary format to be sent or stored.
- Sender - the class responsible for transmitting or storing the converted data.
- App interface - used to interface the application, STL and OS. This provides
  - Possibly custom memory management needed by embedded applications.
  - Task management, including
    - Obtaining byte task IDs required by the Log class.
    - Registering and unregistering the current task.
    - Obtaining the task name in C string.
  - Obtaining timestamps using some OS / STL time routine.
  - Error management.
- Message - used as a transfer medium in the Queue. For numeric types and strings transferred by pointers, one message is needed per item. For stored strings several messages may be necessary. There are two general-purpose implementations:
  - a variant based message.
  - a more space-efficient handcrafted message.

The logger can operate in two modes:
- Direct without queue, when the conversion and sending happens without message instantiation and queue usage. This can be useful for single-threaded applications.
- With queue, when each item sent will form one or more (in case of stored strings) message and use a central thread-safe queue. On the other end of the queue a background thread pops the messages and groups them by tasks in secondary lists. When a group of related items from a task has arrived, its conversion and sending begins.

## Implementations

The current library provides some simple implementations for STL and Boost based desktop and server applications and FreeRTOS based embedded applications.

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
- logging with queue using MessageCompact (for multithreaded applications)
- logging with queue using MessageVariant (for multithreaded applications)

### FreeRTOS with floating point

To obtain net results, I put some floating-point operations in the application test-sizes-freertosminimal-float.cpp because a real application would use them apart of logging. 

|Scenario      |   Text|  Data|    BSS|
|--------------|------:|-----:|------:|
|direct        |13152  | 108  |52     |
|off           |0      |0     |0      |
|MessageVariant|15304  |112   | 76    |
|MessageCompact|15024  |112   | 76    |

### FreeRTOS without floating point

No floating point arithmetics in the application and the support is turned off in the logger.

|Scenario      |   Text|  Data|    BSS|
|--------------|------:|-----:|------:|
|direct        |4303   | 8    |56     |
|off           |0      |0     |0      |
|MessageVariant|6440   |12    | 80    |
|MessageCompact|6192   |12    | 80    |

### x86 STL with floating point

Not much point to calculate size growth here, but why not?

|Scenario      |   Text|  Data|    BSS|
|--------------|------:|-----:|------:|
|direct        |11899  | 273  |492    |
|off           |0      |0     |0      |
|MessageVariant|22483  | 457  |892    |
|MessageCompact|20851  |457   | 892   |

