#ifndef _COMMON_PLATFORM_H
#define _COMMON_PLATFORM_H

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
#define PLATFORM_WINDOWS
#elif defined(__APPLE__) || defined(__MACH__)
#define PLATFORM_DARWIN
#define PLATFORM_POSIX
#elif defined(__linux__)
#define PLATFORM_LINUX
#define PLATFORM_POSIX
#else
#error "Platform not supported"
#endif

#if defined(__cplusplus)
}
#endif

#endif // _COMMON_PLATFORM_H
