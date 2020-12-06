cmake_minimum_required(VERSION 3.17)
project(cpp_logger)

set(CMAKE_CXX_STANDARD 20)

include_directories(cpp-memory-manager)
include_directories(src)

add_compile_options(-ggdb -Wall -Wextra -Wpedantic)
add_link_options(-lpthread)

add_executable(cpp_logger
        cpp-memory-manager/PoolAllocator.h
        src/Log.h
        src/LogAppInterfaceStd.h
        src/LogConverterCustomText.h
        src/LogMessageBase.h
        src/LogMessageCompact.h
        src/LogMessageVariant.h
        src/LogNumericSystem.h
        src/LogQueueStdBoost.h
        src/LogQueueVoid.h
        src/LogSenderStdOstream.h
        src/LogSenderVoid.h
        test/test-stdostream.cpp)
