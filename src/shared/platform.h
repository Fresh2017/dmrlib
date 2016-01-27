#ifndef _SHARED_PLATFORM_H
#define _SHARED_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __MINGW32__

int __winsock_errno(long error);
void __winsock_init(void);

#else // __MINGW32__

#define __winsock_errno(x) (x)
#define __winsock_init()

#endif // __MINGW32__

#ifdef __cplusplus
}
#endif

#endif // _SHARED_PLATFORM_H
