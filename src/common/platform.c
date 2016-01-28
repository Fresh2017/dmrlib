#ifdef __MINGW32__
#include <stdlib.h>
#include <stdbool.h>
#include <winsock2.h>
#include "common/platform.h"

static bool __winsock_init_done = false;

void __winsock_init(void)
{
    if (__winsock_init_done) {
        return;
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) ||
        LOBYTE(wsaData.wVersion) != 2 ||
        HIBYTE(wsaData.wVersion) != 2) {
        exit(111);
    }
    __winsock_init_done = true;
}
#else
void __winsock_init(void) {
    (void)0;
}
#endif // __MINGW32__
