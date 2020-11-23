# C++ template-based log library

This is a complete rework of my [old logger](https://github.com/balazs-bamer/cpp-logger/tree/old/) after identifying the design flaws and performance problems.

The code was written to possibly conform MISRA C++ and High-Integrity C++.

The library is published under the [MIT license](https://opensource.org/licenses/MIT).

Copyright 2020 Balázs Bámer.

Currently there is no user documentation for the new (still work-in-progress) solution, just some hints about the ideas behind the design.

## Aims

- Provide a common logger API for cross-platform embedded / desktop code.
- The library was designed to allow the programmer control log bandwidth usage, program and data memory consumption.
- Log arithmetic C++ types and strings with or without copying them (beneficial for literals).
- We needed a much finer granularity than the usual log levels, so I've introduced independently selectable topics.
- Important aim was to let the user log related items without other tasks interleaving.
- Possibly minimal relying on external libraries. It does not depend for example on the `printf` function family.
- In contrast with the original solution, the new one extensively uses templated classes to
  - implement a flexible plugin architecture for adapting different user needs
  - let the compiler optimizations do their finest.
  - totally eliminate logging code generation with at least -O1 optimization level with a one-line change (no #ifdef).
- Let the user decide to rely on C++ exceptions or implement a different kind of error management.
- Let me learn about recent language features like C++20 concepts (to be implemented).

## Architecture

The logger consists of five classes:
- Log - the main class providing API and base architecture. This accepts the other ones as template parameters.
- Queue - used to transfer the converting and sending operations from the current task to a background one. Can use
  - the faster variant based message, or
  - the slower but more space-efficient handcrafted message.
- Converter - converts the user types to strings or any other binary format to be sent or stored.
- Sender - the class responsible for transmitting or storing the converted data.
- App interface - used to interface the application, STL and OS. This provides
  - Possibly custom memory management needed by embedded applications.
  - Task management.
  - Time management for obtaining timestamps.
  - Error management.
