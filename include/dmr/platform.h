#ifndef _DMR_PLATFORM_H
#define _DMR_PLATFORM_H

#include <inttypes.h>

#if defined(_WIN32) || defined(_WIN64)
#define DMR_PLATFORM_WINDOWS

#elif defined(__APPLE__) || defined(__MACH__)
#define DMR_PLATFORM_DARWIN
#define DMR_PLATFORM_UNIX

#elif defined(__linux__)
#define DMR_PLATFORM_LINUX
#define DMR_PLATFORM_UNIX

#else

#error Platform not supported

#endif

#ifdef DMR_PLATFORM_WINDOWS
#include <windows.h>

// From unistd.h on UNIX
typedef int32_t speed_t;

#ifdef __MINGW32__
//workaround for what I think is a mingw bug: it has a prototype for
//_ftime_s in its headers, but no symbol for it at link time.
//The #define from common.h cannot be used since it breaks other mingw
//headers if any are included after the #define.
#define _ftime_s _ftime
#endif // __MINGW32__

#include <unistd.h>
#define dmr_sleep(s)      Sleep(s * 1000L)
#define dmr_msleep(ms)    Sleep(ms)
#define dmr_usleep(us)    Sleep(us / 1000L)

#endif // DMR_PLATFORM_WINDOWS

#ifdef DMR_PLATFORM_UNIX

#include <unistd.h>
#define dmr_sleep(s)      sleep(s)
#define dmr_msleep(s)     usleep(s * 1000L)
#define dmr_usleep(s)     usleep(s)

#endif // DMR_PLATFORM_UNIX

#endif // _DMR_PLATFORM_H
