#pragma once

#include <cassert>
#include <cstdio>

#define TEAL "\033[36m"
#define WHITE "\033[1;37m"
#define YELLOW "\033[1;33m"
#define RED "\033[1;31m"
#define PURPLE "\033[1;35m"

#define LTRACE_ARM(format, ...) printf("trace: %08X: %08X  " format "\033[0m\n", GetPC() - 8, opcode, ##__VA_ARGS__)
#define LTRACE_THUMB(format, ...) printf("trace: %08X: %04X      " format "\033[0m\n", GetPC() - 4, opcode, ##__VA_ARGS__)
#define LTRACE_DOUBLETHUMB(format, ...) printf("trace: %08X: %08X  " format "\033[0m\n", GetPC() - 4, double_opcode, ##__VA_ARGS__)
#define LDEBUG(format, ...) printf(TEAL "debug: " format "\033[0m\n", ##__VA_ARGS__)
#define LINFO(format, ...) printf(WHITE "info: " format "\033[0m\n", ##__VA_ARGS__)
#define LWARN(format, ...) printf(YELLOW "warning: " format "\033[0m\n", ##__VA_ARGS__)
#define LERROR(format, ...) printf(RED "error: " format "\033[0m\n", ##__VA_ARGS__)
#define LFATAL(format, ...) printf(PURPLE "fatal: " format "\033[0m\n", ##__VA_ARGS__)

#define UNIMPLEMENTED() \
    LFATAL("unimplemented code at %s:%u", __FILE__, __LINE__); \
    assert(false); \
    std::exit(1); // exit just in case assert doesn't

#define UNIMPLEMENTED_MSG(format, ...) \
    LFATAL("unimplemented code at %s:%u", __FILE__, __LINE__); \
    LFATAL(format, ##__VA_ARGS__); \
    assert(false); \
    std::exit(1); // exit just in case assert doesn't

#define UNREACHABLE() \
    LFATAL("unreachable code at %s:%u", __FILE__, __LINE__); \
    assert(false); \
    std::exit(1); // exit just in case assert doesn't

#define UNREACHABLE_MSG(format, ...) \
    LFATAL("unreachable code at %s:%u", __FILE__, __LINE__); \
    LFATAL(format, ##__VA_ARGS__); \
    assert(false); \
    std::exit(1); // exit just in case assert doesn't

#define ASSERT(cond) \
    if (!(cond)) { \
        LFATAL("assertion failed at %s:%u", __FILE__, __LINE__); \
        LFATAL("%s", #cond); \
        assert(cond); \
        std::exit(1); \
    }

#define ASSERT_MSG(cond, format, ...) \
    if (!(cond)) { \
        LFATAL("assertion failed at %s:%u", __FILE__, __LINE__); \
        LFATAL(format, ##__VA_ARGS__); \
        assert(cond); \
        std::exit(1); \
    }
