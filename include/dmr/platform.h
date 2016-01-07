#ifndef _DMR_PLATFORM_H
#define _DMR_PLATFORM_H

#include <inttypes.h>

#if defined(_WIN32) || defined(_WIN64)

#define DMR_PLATFORM_WINDOWS

#include <windows.h>

// From unistd.h on UNIX
typedef int32_t speed_t;

#elif defined(__APPLE__) || defined(__MACH__)

#define DMR_PLATFORM_DARWIN
#define DMR_PLATFORM_UNIX

#elif defined(__linux__)

#define DMR_PLATFORM_LINUX
#define DMR_PLATFORM_UNIX

#else

#error Platform not supported

#endif

#endif // _DMR_PLATFORM_H
