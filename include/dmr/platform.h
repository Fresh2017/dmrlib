#ifndef _DMR_PLATFORM_H
#define _DMR_PLATFORM_H

#include <inttypes.h>

#if defined(_WIN32) || defined(_WIN64)

#define DMR_PLATFORM_WINDOWS

#include <windows.h>

// From unistd.h on UNIX
typedef int32_t speed_t;

#ifdef __MINGW32__
//workaround for what I think is a mingw bug: it has a prototype for
//_ftime_s in its headers, but no symbol for it at link time.
//The #define from common.h cannot be used since it breaks other mingw
//headers if any are included after the #define.
#define _ftime_s _ftime
#endif

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
