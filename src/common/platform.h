#ifndef _COMMON_PLATFORM_H
#define _COMMON_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__MINGW32__)

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif

int __winsock_errno(long error);
void __winsock_init(void);

#else // __MINGW32__

#define __winsock_errno(x) (x)
#define __winsock_init()

#endif // __MINGW32__

#ifdef __cplusplus
}
#endif

#endif // _COMMON_PLATFORM_H
