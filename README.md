# C++ template-based log library

Originally, this library was designed to use in an embedded environment as a debugging log library.
It was designed to be lean and versatile
with different options for tailoring the logger behavior for smaller
or larger MCUs. 

There are two log call flavours:
  - The function call-like solution with only one value to print per line. For small compiled size, avoid many parameter footprints (i.e. avoid many template instantiations).
  - The `std::ostream`-like solution, which uses minimal stack. Discarding headers is interesting only if there are many simple homogeneous calls, or the available bandwidth is limited.

The library reserves some buffers and a queue during construction, and makes
small heap allocations during thread / log topic registrations. After it, no heap
modification occurs, so it will be safe to use from embedded code.

The code was written to possibly conform MISRA C++ and High-Integrity
C++.

The library is published under the [MIT license](https://opensource.org/licenses/MIT).

Copyright 2018-2020 Now Technologies Zrt.

## Resource consumption

The library was designed to allow the programmer control log bandwidth usage, program and data memory consumption with minimal relying on external libraries. It does not depend for example on the `printf` function family. Enabling logging in this example program has the following memory usages on STM32F2xx using g++ 5.4.1:

```cpp
extern "C" void appMain() noexcept {
  static nowtech::LogConfig config;
  static nowtech::LogStmHal osInterface(&huart2, config, 100);
  static Log log(osInterface, config);
  Log::registerCurrentTask();
  for(;;) {
    uint32_t result;
    if(HAL_RNG_GenerateRandomNumber(&hrng, &result) == HAL_OK) {
      //Log::sendNoHeader(LC::cX8, result);
      Log::n() << LC::cX8 << result << Log::end;
    }
  }
}
```

Note, this example uses the minimal interface for UART output, without the FreeRTOS multitasking support. (The example doesn't have FreeRTOS at all.) Increase using the variadic template call (in comment in the example):

|Area |Inrease in debug mode |Increase in release mode (-Os)|
|-----|---------------------:|-----------------------------:|
|prg  |14784                 |4460                          |
|data |72                    |68                            |
|bss  |112                   |108                           |

Increase using the chained call (effective in the example):

|Area |Inrease in debug mode |Increase in release mode (-Os)|
|-----|---------------------:|-----------------------------:|
|prg  |15164                 |4628                          |
|data |72                    |68                            |
|bss  |112                   |108                           |

## Log example

This is a FreeRTOS multithreaded log example with and without header info (task name, uptime in ms, log topic). The header components can individually be switched on and off.

```defaultThread 0000001 SYSTEM       Reset cause: brown out
defaultThread 0000001 PERSISTENCE  Calculated persistence length: 89
watchdogThrd. 0000006 WATCHDOG     Registered [0]: black box    
defaultThread 0000117 BLACKBOX     start: 180 capac: 223 low: 44 intact: 89
defaultThread 0000278 BLACKBOX     expecting 223 records.
defaultThread 0000279 BLACKBOX     read 223 valid records.
0 0. rec: 0 RstSrcOpTimeIv 2245m, reset: 01000110, invalid: false
defaultThread 0000279 PERSISTENCE  readValues: copy 0
defaultThread 0000288 PERSISTENCE  readValues: copy 0: no mismatch
defaultThread 0000288 BLACKBOX     start: 180 capac: 223 low: 44 intact: 89
defaultThread 0000297 BLACKBOX     last session:
46 226. rec: 0 RstSrcOpTimeIv 4424m, reset: 00001010, invalid: false
44 227. rec: 0 PanBldRelCharg PAN: 307, build: 1040, release: true charge: true
43 228. rec: 3 ServModeEnter  164850ms, arg: 0
```

## API

The library is divided into two classes:

  - `nowtech::Log` for high-level API and template logic.
  - `nowtech::LogOsInterface` and subclasses for OS or library-dependent layer.

The `nowtech::Log` class provides a high-level template based interface for logging characters, C-style
strings, signed and unsigned integers and floating point types. This
class was designed to use with 32 bit MCUs with small memory, performs
serialization and number conversion itself. It uses a heap-allocated
buffers and requires logging threads to register themselves. To ensure
there is enough memory for all application functions, object
construction and thread registration should be done as early as
possible.

Log topic values of type `nowtech::LogTopicInstance` (one for each registered topic) are intended to be stored in some global accessible construct for convenient use. Of course, local storage and passing values around is also possible. The registration mechanism allows libraries to define their own topic variable set and be compiled on its own into a static/dynamic library. Only the registration is required to be performed in the initialization part of the application. An appropriate header file would look like

```cpp
#ifndef SOMELOGTOPICS_H_INCLUDED
#define SOMELOGTOPICS_H_INCLUDED

#include "Log.h"

namespace nowtech {
namespace SomeLogTopicNamespace { 

extern LogTopicInstance system;
extern LogTopicInstance watchdog;
extern LogTopicInstance selfTest;

}
}

#endif /* SOMELOGTOPICS_H_INCLUDED */
```

While the .cpp file

```cpp
#include "LogTopics.h"

nowtech::LogTopicInstance nowtech::SomeLogTopicNamespace::system;
nowtech::LogTopicInstance nowtech::SomeLogTopicNamespace::watchdog;
nowtech::LogTopicInstance nowtech::SomeLogTopicNamespace::selfTest;
```


### The class operates roughly as follows:

The message is constructed on the caller stack and is broken into
chunks identified by the artificially created task ID.
Therefore, all tasks should register themselves on the very beginning
of their life. Calling from ISRs is also possible, but all such calls will get the
same ID.

These resulting chunks (log message parts) are then fed into a
common queue, either blocking to wait for free place in the queue or letting
chunks dropped. Concurrent loggings can get their chunks interleaved
in the queue. If the queue becomes full, chunks will be dropped, which
results in corrupted messages.

An algorithm ensures the messages are de-interleaved in the order of
their first chunk in the queue and fed into one of the two transmission buffers,
while the other one may be transmitted. A timeout value is
used to ensure transmission in a defined amount of time even if the
transmission buffer is not full.  
This class can have a stub implementation in an other .cpp file to
prevent logging in release without the need of #ifdefs and macros.

The Log class shall have only one instance, and can be accessed via a
static method.  
If code size matters, the template argument pattern should come from a
limited= number parameter type combinations, and possibly only a
small number of parameters.

### Log initialization

The following steps are required to initialize the log system:

1.  Create a `nowtech::LogConfig` object with the desired settings.
    This instance should not be destructed.
2.  Create an object of the desired `Nowtech::LogOsInterface` subclass. This instance should not be destructed.
3.  Create the Log instance using the above two objects. This instance should not be destructed.
4.  Register the required topics using `Log::registerTopic` to let only a
    subset of logs be printed. These variables of type `nowtech::LogTopicInstance` are defined in various parts of the whole application, even in libraries. For example, if
    you have there `system`, `connection` and `watchdog` defined, and
    you only register `connection` and `watchdog`, all the calls
    starting with `Log::send(nowtech::SomeLogTopicNameSpace::system` will be
    discarded. Registration is performed like `Log::registerTopic(nowtech::SomeLogTopicNamespace::system, "system");`

5.  Call `Log::registerCurrentTask();` in all the tasks you want to log
    from. This is necessary for the log system to avoid message interleaving from different tasks.

Note, the Log system behaves as a singleton, and defining two instances won't work.

### Log configuration

Log message format can be controlled

  - by the contents of the `LogConfig` object used during initialization. These are application-wide settings.
  - by the actual method template called and its parameters. These control only this message or one of its parameters.

Format of numeric values are described by `nowtech::LogFormat`. Its
constructor takes two parameters:

  - base: numeric system base, only effective for integer numbers. Can
    be 2, 10 or 16, all other values result in error message. Floating
    point numbers always use 10.
  - fill: number of digits to emit with zero fill, or 0 if no fill.

#### LogConfig

The default values defined in this struct are good for general-purpose
use.

Field | Possible values | Default value | Effect
------|-----------------|---------------|--------
`cDefault`|constant     |LogFormat(10, 0)|This is the default logging format.
`cNone`|constant        |LogFormat(0, 0)|Can be used to omit printing the next parameter = (-:
`cBn` |constant         |LogFormat(2, *n*)|Used for n-bit binary output, where *n* can be 8, 16, 24 or 32.
cD*n* |constant         |LogFormat(10, *n*)|Used for n-digit decimal output, where n can be 2-8.
cX*n* |constant         |LogFormat(16, *n*)|Used for n-digit hexadecimal output, where *n* can be 2, 4, 6 or 8.
allowRegistrationLog|bool|true          |If true, task registration will be sent to the output in the form -=- Registered task: taskname (1) -=- **Note**, systems with limited stack space and using std::ostream-like calls need to disable this, because the output is created using the stack-hungry variadic template call.
`logFromIsr`|bool       |false          |If false, log calls from ISR are discarded. If true, logging from ISR works. However, in this mode the message may be truncated if the actual free space in the queue is too small.
`chunkSize`|uint32_t    |8              |Total message chunk size to use in queue and buffers. The net capacity is one less, because the task ID takes a character. Messages are not handled as a string of characters, but as a series of chunks. '\\r' signs the end of a message.
`queueLength`|uint32_t  |64             |Length of a queue in chunks. Increasing this value decreases the probability of message truncation when the queue stores more chunks.
`circularBufferLength`|uint32_t|64      |Length of the circular buffer used for message sorting, measured also in chunks. This should have the same length as the queue, but one can experiment with it.
`transmitBufferLength`|uint32_t|32      |Length of a buffer in the transmission double-buffer pair, in chunks. This should have half the length as the queue, but one can experiment with it. To be absolutely sure, this can have the same length as the queue, and the log system will also manage bursts of logs.
`appendStackBufferLength`|uint16_t|34   |Length of stack-reserved buffer for number to string conversion. The default value is big enough to hold 32 bit binary numbers. Can be reduced if no binary output is used and stack space is limited.
`pauseLength`|uint32_t|100              |Length of a pause in ms during waiting for transmission of the other buffer or timeout while reading from the queue.
`refreshPeriod`|uint32_t|100            |Length of the period used to wait for messages before transmitting a partially filled transmission buffer. The shorter the value the more prompt the display.
`blocks`|bool           |true           |Signs if writing the queue from tasks can block or should return on the expense of possibly losing chunks. Note, that even in blocking mode the throughput can not reach the theoretical throughput (such as UART bps limit). **Important\!** In non-blocking mode high demands will result in loss of complete messages and often an internal lockup of the log system.
`taskRepresentation`|`cNone`, `cId`, `cName`|TaskRepresentation::cId|Representation of a task in the message header, if any. It can be missing, numeric task ID or OS task name.
`appendBasePrefix`|bool |false          |True if number formatter should append 0b or 0x.
`taskIdFormat`|see LogFormat above|`cX2`|Format for displaying the task ID in the message header, if it is displayed as ID.
`tickFormat`|see LogFormat above|`cD5`|Format for displaying the OS ticks in the header, if any. Should be `LogFormat::cNone` to disable tick output.
`int8Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`int16Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`int32Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`int64Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`uint8Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`uint16Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`uint32Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`uint64Format`|see LogFormat above|`cDefault`|Applies to numeric parameters of this type without preceding format parameter.
`floatFormat`|see LogFormat above|`cD5`|Applies to numeric parameters of this type without preceding format parameter.
`doubleFormat`|see LogFormat above|`cD8`|Applies to numeric parameters of this type without preceding format parameter.
`alignSigned`|bool      |false          |If true, positive numbers will be prepended with a space to let them align negatives.

### Invocation

#### Function call-like solution

The API has 8 static method templates, which lets the log system be
called from any place in the application:

  - `static void send(LogTopicType aTopic, LogFormat const &aFormat, tValueType const aValue) noexcept;`
  - `static void send(LogTopicType aTopic, tValueType const aValue) noexcept;`
  - `static void send(LogFormat const &aFormat, tValueType const aValue) noexcept;`
  - `static void send(tValueType const aValue) noexcept;`
  - `static void sendNoHeader(LogTopicType aTopic, LogFormat const &aFormat, tValueType const aValue) noexcept;`
  - `static void sendNoHeader(LogTopicType aTopic, tValueType const aValue) noexcept;`
  - `static void sendNoHeader(LogFormat const &aFormat, tValueType const aValue) noexcept;`
  - `static void sendNoHeader(tValueType const aValue) noexcept;`

The ones with the name `send` behave according to the stored configuration and the actual parameter list.
The ones with the name `sendNoHeader` skip printing a header, if any defined in the config.
The ones receiving the `LogTopicType` parameter will emit the message
preceded by the string used to register the given `LogTopicInstance` parameter
if it was actually registered. If not, the whole message is discarded.
Passing the `LogTopicInstance` variables is not possible, but this class has an overloaded * paramerer to return the contained `LogTopicType` value.
The ones without the `LogTopicType` parameter will emit the message
unconditionally.

Examples:
```cpp
Log::send(*nowtech::SomeLogTopicNamespace::system, int64);
Log::send(uint64);
Log::sendNoHeader(*nowtech::SomeLogTopicNamespace::system, LC::cD7, int64);
```

#### std::ostream-like solution

The following entry points are available:
  - Singleton access:
    - `static LogShiftChainHelper i() noexcept;` -- prints header, logs unconditionally
    - `static LogShiftChainHelper i(LogTopicType const aTopic) noexcept;` -- logs with header if the topic is enabled
    - `static LogShiftChainHelper n() noexcept;` -- doesn't print header, logs unconditionally
    - `static LogShiftChainHelper n(LogTopicType const aTopic) noexcept;` -- logs without header if the topic is enabled
  - Member access (no header surpression possible):
    - `LogShiftChainHelper operator<<(ArgumentType const aValue) noexcept;` -- not recommended, the singleton version will avoid unnecessary template instantiations
    - `LogShiftChainHelper operator<<(LogTopicType const aTopic) noexcept;` -- logs with header if the topic is enabled
    - `LogShiftChainHelper operator<<(LogFormat const &aFormat) noexcept;` -- logs with header unconditionally
    - `LogShiftChainHelper operator<<(LogShiftChainMarker const aMarker) noexcept;` -- just for fun, does nothing

The class `LogTopicInstance` has an overloaded cast operator to `LogTopicType` so it can be passed directly. Due to operator overloading, static access is not available, so the Log instance has to be required:

```cpp
Log::i() << uint8 << ' ' << int8 << Log::end;
Log::i() << nowtech::SomeLogTopicNamespace::system << uint8 << ' ' << int8 << Log::end;
Log::i() << LC::cX2 << uint8 << int8 << Log::end;
Log::i() << Log::end;
```

**Important**: omitting Log::end from the end of the clause makes the logged content mix with contents from subsequent calls. Of course, if the once created instance is available at the call location, it can be used instead of the static call returning the same singleton.

This solution even allows distributed construction of a log line. In this example I write 16 bytes to a line in a way that permits arrays with size not multiple of 16 to be written:

```cpp
auto log = Log::i(nowtech::SomeLogTopicNamespace::system);
int32_t i;
for(i = 0; i < chunkLengthOut; ++i) {
  if(i > 0 && i % 16 == 0) {
    log << Log::end;
    // some wait may be needed for long arrays
    log = Log::i(nowtech::SomeLogTopicNamespace::system);
  }
  else { // nothing to do
  }
  log << LC::cX2 << puffer[i] << ' ';
}
log << Log::end;
```

**Important**: there is, however, no mechanism implemented to manage or inhibit creating two simultaneous such sessions from the same thread. Such a mechanism would require an additional array with more memory requirement. It is the application writer's responsibility to prevent such situations.

Available parameter types to print:

Type        |Printed value          |If it can be preceded by a LogFormat parameter to change the style
------------|-----------------------|-------------------------------------------------------------------
`bool`      |false / true           |no
`char`      |the character, \r character not allowed           |no
`char*`     |the 0 terminated string, \r characters not allowed|no
`uint8_t`   |formatted numeric value|yes
`uint16_t`  |formatted numeric value|yes
`uint32_t`  |formatted numeric value|yes
`uint64_t`  |formatted numeric value|yes
`int8_t`    |formatted numeric value|yes
`int16_t`   |formatted numeric value|yes
`int32_t`   |formatted numeric value|yes
`int64_t`   |formatted numeric value|yes
`float`     |formatted numeric value in exponential form|yes
`double`    |formatted numeric value in exponential form|yes
anything else, like pure `int`|`-=unknown=-`|no

The logger was initially designed for 32-bit embedded environment with possible few binary-to-printed
converter template function instantiation. From 8 to 32 bit numbers only the 32-bit versions will be created.
Using 64-bit numbers makes the compiler create the 64-bit version(s) as well, depending on the signedness
of the numbers to log.

## OS interface

Abstract base class for OS/architecture-dependent log functionality
under the Log class. Since this object requires OS resources, it must be constructed
during initialization of the application to be sure all resources are
granted.

### Implementations

There are several pluggable OS interfaces.

Header name            |Target          |Extensively tested|Description
-----------------------|----------------|------------------|------------
LogNop.h               |imaginary /dev/null|yes            |No-operation interface for no output at all. Can be used to appearantly shut down logging at compile time.
LogStmHal.h            |An STM HAL UART device|yes         |An interface for STM HAL making immediate transmits from the actual thread. This comes without any buffering or concurrency support, so messages from different threads may interleave each other.
LogFreertosStmHal.h    |An STM HAL UART device|yes         |An interface for STM HAL under FreeRTOS, tested with version 9.0.0. This implementaiton is designed to put as little load on the actual thread as possible. It makes use of the built-in buffering and transmits from its own thread.
LogFreertosBlocking.h  |Any blocking device|yes            |An interface for any blocking transmission device under FreeRTOS, tested with version 9.0.0. It makes use of the built-in buffering and transmits from its own thread.
LogCmsisSwo.h          |CMSIS SWO       |not yet           |An interface for CMSIS SWO making immediate transmits from the actual thread. This comes without any buffering or concurrency support, so messages from different threads may interleave each other.
LogFreertosCmsisSwo.h  |CMSIS SWO       |not yet           |An interface for CMSIS SWO under FreeRTOS, tested with version 9.0.0. This implementaiton is designed to put as little load on the actual thread as possible. It makes use of the built-in buffering and transmits from its own thread.
LogStdOstream.h        |std::ostream    |yet               |An interface for std::ostream making immediate transmits from the actual thread. This comes without any buffering or concurrency support, so messages from different threads may interleave each other.
LogStdThreadOstream.h  |std::ostream    |yes               |An interface using STL (even for threads) and boost::lockfree::queue. Thanks to this class, this implementation is lock-free. Note, this class does not own the std::ostream and does nothing but writes to it. Opening, closing etc is responsibility of the user code. The stream should NOT throw exceptions. Note, as this interface does not know interrupts, skipping a thread registration will prevent logging from that thread. Note, this class **requires Boost** to compile.

## Compiling

The compiler must support the C++14 standard.

Compulsory files are:
  - BanCopyMove.h
  - CmsisOsUtils.cpp
  - CmsisOsUtils.h
  - Log.cpp
  - Log.h
  - LogUtil.cpp
  - LogUtil.h

One of these headers, and if present the related .cpp is also needed:
  - LogNop.h
  - LogStmhal.h
  - LogFreertosStmhal.h
  - LogFreertosStmhal.cpp
  - LogFreertosBlocking.h
  - LogFreertosBlocking.cpp
  - LogCmsisSwo.h
  - LogFreertosCmsisSwo.h
  - LogFreertosCmsisSwo.cpp
  - LogStdOstream.h
  - LogStdThreadOstream.h
  - LogStdThreadOstream.cpp

_**Missing** files are_:
  - stm32hal.h - this is a placeholder for a set of includes like `stm32f215xx.h`, `stm32f2xx_hal.h`, `stm32f2xx_ll_utils.h` for a given MCU.
  - stm32utils.h which should contain a function for interrupt testing, like

```cpp
namespace stm32utils {
  inline bool isInterrupt() noexcept {
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
  }
}
```

Some interfaces need HAL callbacks. `logfreertosstmhal.h` needs such a function:

```cpp
extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if(huart == &huart3) {
    nowtech::logfreertosstmhal::transmitFinished();
  }
  else { // nothing to do
  }
}
```

`logfreertosstmhal.h` needs a similar one with the corresponding class name.

## TODO

  - Eliminate std::map.
  - Fix the lockup bug happening under extreme loads.
