#pragma once

#include <cassert>
#include <fmt/color.h>
#include <fmt/core.h>

#define LTRACE_ARM(format, ...) fmt::print("trace: {:08X}: {:08X}  " format "\n", GetPC() - 8, opcode, ##__VA_ARGS__)
#define LTRACE_THUMB(format, ...) fmt::print("trace: {:08X}: {:04X}      " format "\n", GetPC() - 4, opcode, ##__VA_ARGS__)
#define LTRACE_DOUBLETHUMB(format, ...) fmt::print("trace: {:08X}: {:08X}  " format "\n", GetPC() - 4, double_opcode, ##__VA_ARGS__)
#define LDEBUG(format, ...) fmt::print(fg(fmt::color::teal), "debug: " format "\n", ##__VA_ARGS__)
#define LINFO(format, ...) fmt::print(fmt::emphasis::bold | fg(fmt::color::white), "info: " format "\n", ##__VA_ARGS__)
#define LWARN(format, ...) fmt::print(fmt::emphasis::bold | fg(fmt::color::yellow), "warning: " format "\n", ##__VA_ARGS__)
#define LERROR(format, ...) fmt::print(fmt::emphasis::bold | fg(fmt::color::red), "error: " format "\n", ##__VA_ARGS__)
#define LFATAL(format, ...) fmt::print(fmt::emphasis::bold | fg(fmt::color::fuchsia), "fatal: " format "\n", ##__VA_ARGS__)

#define UNIMPLEMENTED() \
    LFATAL("unimplemented code at {}:{}", __FILE__, __LINE__); \
    assert(false); \
    std::exit(1); // exit just in case assert doesn't

#define UNIMPLEMENTED_MSG(format, ...) \
    LFATAL("unimplemented code at {}:{}", __FILE__, __LINE__); \
    LFATAL(format, ##__VA_ARGS__); \
    assert(false); \
    std::exit(1); // exit just in case assert doesn't

#define UNREACHABLE() \
    LFATAL("unreachable code at {}:{}", __FILE__, __LINE__); \
    assert(false); \
    std::exit(1); // exit just in case assert doesn't

#define UNREACHABLE_MSG(format, ...) \
    LFATAL("unreachable code at {}:{}", __FILE__, __LINE__); \
    LFATAL(format, ##__VA_ARGS__); \
    assert(false); \
    std::exit(1); // exit just in case assert doesn't

#define ASSERT(cond) \
    if (!(cond)) { \
        LFATAL("assertion failed at {}:{}", __FILE__, __LINE__); \
        LFATAL("{}", #cond); \
        assert(cond); \
        std::exit(1); \
    }

#define ASSERT_MSG(cond, format, ...) \
    if (!(cond)) { \
        LFATAL("assertion failed at {}:{}", __FILE__, __LINE__); \
        LFATAL(format, ##__VA_ARGS__); \
        assert(cond); \
        std::exit(1); \
    }
