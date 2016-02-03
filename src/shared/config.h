#ifndef _DMR_CONFIG_H
#define _DMR_CONFIG_H

#undef DMR_DEBUG

#undef WITH_DMALLOC

#undef WITH_MBELIB

#if !defined(HAVE_ARPA_INET_H)
#define HAVE_ARPA_INET_H
#endif

#if !defined(HAVE_CTYPE_H)
#define HAVE_CTYPE_H
#endif

#if !defined(HAVE_EPOLL)
#define HAVE_EPOLL
#endif

#if !defined(HAVE_ERRNO_H)
#define HAVE_ERRNO_H
#endif

#if !defined(HAVE_INLINE)
#define HAVE_INLINE
#endif

#if !defined(HAVE_INTTYPES_H)
#define HAVE_INTTYPES_H
#endif

#if !defined(HAVE_LIBBSD)
#define HAVE_LIBBSD
#endif

#if !defined(HAVE_LIBC_IPV6)
#define HAVE_LIBC_IPV6
#endif

#if !defined(HAVE_LIBC_SCOPE_ID)
#define HAVE_LIBC_SCOPE_ID
#endif

#if !defined(HAVE_LIBGEN_H)
#define HAVE_LIBGEN_H
#endif

#if !defined(HAVE_LIBM)
#define HAVE_LIBM
#endif

#if !defined(HAVE_LIBPCAP)
#define HAVE_LIBPCAP
#endif

#if !defined(HAVE_LIBPORTAUDIO)
#define HAVE_LIBPORTAUDIO
#endif

#if !defined(HAVE_LIBPTHREAD)
#define HAVE_LIBPTHREAD
#endif

#if !defined(HAVE_LIBTALLOC)
#define HAVE_LIBTALLOC
#endif

#if !defined(HAVE_LUA)
#define HAVE_LUA
#endif

#if !defined(HAVE_NETINET_IP_H)
#define HAVE_NETINET_IP_H
#endif

#if !defined(HAVE_NETINET_UDP_H)
#define HAVE_NETINET_UDP_H
#endif

#if !defined(HAVE_NET_ETHERNET_H)
#define HAVE_NET_ETHERNET_H
#endif

#if !defined(HAVE_PKG_CONFIG)
#define HAVE_PKG_CONFIG
#endif

#if !defined(HAVE_POLL)
#define HAVE_POLL
#endif

#if !defined(HAVE_RESTRICT)
#define HAVE_RESTRICT
#endif

#if !defined(HAVE_SELECT)
#define HAVE_SELECT
#endif

#if !defined(HAVE_SIGNAL_H)
#define HAVE_SIGNAL_H
#endif

#if !defined(HAVE_SO_REUSEADDR)
#define HAVE_SO_REUSEADDR
#endif

#if !defined(HAVE_SO_REUSEPORT)
#define HAVE_SO_REUSEPORT
#endif

#if !defined(HAVE_STDARG_H)
#define HAVE_STDARG_H
#endif

#if !defined(HAVE_STDBOOL_H)
#define HAVE_STDBOOL_H
#endif

#if !defined(HAVE_STDDEF_H)
#define HAVE_STDDEF_H
#endif

#if !defined(HAVE_STDIO_H)
#define HAVE_STDIO_H
#endif

#if !defined(HAVE_STDLIB_H)
#define HAVE_STDLIB_H
#endif

#if !defined(HAVE_STRING_H)
#define HAVE_STRING_H
#endif

#if !defined(HAVE_SYS_PARAM_H)
#define HAVE_SYS_PARAM_H
#endif

#if !defined(HAVE_TIME_H)
#define HAVE_TIME_H
#endif

#if !defined(HAVE_LIBPTHREAD)
#define DMR_HAVE_PTHREAD
#endif

#endif // _DMR_CONFIG_H