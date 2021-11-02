#pragma once

#include <cassert>
#include <source_location>
#include <fmt/color.h>
#include <fmt/core.h>

#define LTRACE_ARM(format, ...) fmt::print("trace: {:08X}: {:08X}  " format "\n", GetPC() - 8, opcode, ##__VA_ARGS__)
#define LTRACE_THUMB(format, ...) fmt::print("trace: {:08X}: {:04X}      " format "\n", GetPC() - 4, opcode, ##__VA_ARGS__)
#define LTRACE_DOUBLETHUMB(format, ...) fmt::print("trace: {:08X}: {:08X}  " format "\n", GetPC() - 4, double_opcode, ##__VA_ARGS__)
#define LDEBUG(format, ...) // fmt::print(fg(fmt::color::teal), "debug: " format "\n", ##__VA_ARGS__)
#define LINFO(format, ...) fmt::print(fmt::emphasis::bold | fg(fmt::color::white), "info: " format "\n", ##__VA_ARGS__)
#define LWARN(format, ...) fmt::print(fmt::emphasis::bold | fg(fmt::color::yellow), "warning: " format "\n", ##__VA_ARGS__)
#define LERROR(format, ...) fmt::print(fmt::emphasis::bold | fg(fmt::color::red), "error: " format "\n", ##__VA_ARGS__)
#define LFATAL(format, ...) fmt::print(fmt::emphasis::bold | fg(fmt::color::fuchsia), "fatal: " format "\n", ##__VA_ARGS__)

#define UNIMPLEMENTED() \
    constexpr std::source_location sl = std::source_location::current(); \
    LFATAL("unimplemented code at {}:{}", sl.file_name(), sl.line()); \
    abort(); \

#define UNIMPLEMENTED_MSG(format, ...) \
    constexpr std::source_location sl = std::source_location::current(); \
    LFATAL("unimplemented code at {}:{}", sl.file_name(), sl.line()); \
    LFATAL(format, ##__VA_ARGS__); \
    abort(); \

#define UNREACHABLE() \
    constexpr std::source_location sl = std::source_location::current(); \
    LFATAL("unreachable code at {}:{}", sl.file_name(), sl.line()); \
    abort(); \

#define UNREACHABLE_MSG(format, ...) \
    constexpr std::source_location sl = std::source_location::current(); \
    LFATAL("unreachable code at {}:{}", sl.file_name(), sl.line()); \
    LFATAL(format, ##__VA_ARGS__); \
    abort(); \

#define ASSERT(cond) \
    if (!(cond)) { \
        constexpr std::source_location sl = std::source_location::current(); \
        LFATAL("assertion failed at {}:{}", sl.file_name(), sl.line()); \
        LFATAL("{}", #cond); \
        assert(cond); \
        std::exit(1); \
    }

#define ASSERT_MSG(cond, format, ...) \
    if (!(cond)) { \
        constexpr std::source_location sl = std::source_location::current(); \
        LFATAL("assertion failed at {}:{}", sl.file_name(), sl.line()); \
        LFATAL(format, ##__VA_ARGS__); \
        assert(cond); \
        std::exit(1); \
    }
