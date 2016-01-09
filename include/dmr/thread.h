/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
Copyright (c) 2012 Marcus Geelnard
Copyright (c) 2013-2014 Evan Nemerson

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

#ifndef _DMR_THREAD_H
#define _DMR_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Which platform are we on? */
#if !defined(_DMR_THREAD_PLATFORM_DEFINED_)
  #if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
    #define _DMR_THREAD_WIN32_
  #else
    #define _DMR_THREAD_POSIX_
  #endif
  #define _DMR_THREAD_PLATFORM_DEFINED_
#endif

/* Activate some POSIX functionality (e.g. clock_gettime and recursive mutexes) */
#if defined(_DMR_THREAD_POSIX_)
  #undef _FEATURES_H
  #if !defined(_GNU_SOURCE)
    #define _GNU_SOURCE
  #endif
  #if !defined(_POSIX_C_SOURCE) || ((_POSIX_C_SOURCE - 0) < 199309L)
    #undef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 199309L
  #endif
  #if !defined(_XOPEN_SOURCE) || ((_XOPEN_SOURCE - 0) < 500)
    #undef _XOPEN_SOURCE
    #define _XOPEN_SOURCE 500
  #endif
#endif

/* Generic includes */
#include <time.h>

/* Platform specific includes */
#if defined(_DMR_THREAD_POSIX_)
  #include <pthread.h>
#elif defined(_DMR_THREAD_WIN32_)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #define __UNDEF_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #ifdef __UNDEF_LEAN_AND_MEAN
    #undef WIN32_LEAN_AND_MEAN
    #undef __UNDEF_LEAN_AND_MEAN
  #endif
#endif

/* Compiler-specific information */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
  #define DMR_THREAD_NORETURN _Noreturn
#elif defined(__GNUC__)
  #define DMR_THREAD_NORETURN __attribute__((__noreturn__))
#else
  #define DMR_THREAD_NORETURN
#endif

/* If TIME_UTC is missing, provide it and provide a wrapper for
   timespec_get. */
#ifndef TIME_UTC
#define TIME_UTC 1
#define _DMR_THREAD_EMULATE_TIMESPEC_GET_

#if defined(_DMR_THREAD_WIN32_)
struct _dmr_thread_timespec {
  time_t tv_sec;
  long   tv_nsec;
};
#define timespec _dmr_thread_timespec
#endif

int _dmr_thread_timespec_get(struct timespec *ts, int base);
#define timespec_get _dmr_thread_timespec_get
#endif

/** TinyCThread version (major number). */
#define TINYCTHREAD_VERSION_MAJOR 1
/** TinyCThread version (minor number). */
#define TINYCTHREAD_VERSION_MINOR 2
/** TinyCThread version (full version). */
#define TINYCTHREAD_VERSION (TINYCTHREAD_VERSION_MAJOR * 100 + TINYCTHREAD_VERSION_MINOR)

/**
* @def _dmr_thread_local
* Thread local storage keyword.
* A variable that is declared with the @c _dmr_thread_local keyword makes the
* value of the variable local to each thread (known as thread-local storage,
* or TLS). Example usage:
* @code
* // This variable is local to each thread.
* _dmr_thread_local int variable;
* @endcode
* @note The @c _dmr_thread_local keyword is a macro that maps to the corresponding
* compiler directive (e.g. @c __declspec(thread)).
* @note This directive is currently not supported on Mac OS X (it will give
* a compiler error), since compile-time TLS is not supported in the Mac OS X
* executable format. Also, some older versions of MinGW (before GCC 4.x) do
* not support this directive, nor does the Tiny C Compiler.
* @hideinitializer
*/

#if !(defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201102L)) && !defined(_dmr_thread_local)
 #if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined(__IBMCPP__)
  #define _dmr_thread_local __thread
 #else
  #define _dmr_thread_local __declspec(thread)
 #endif
#elif defined(__GNUC__) && defined(__GNUC_MINOR__) && (((__GNUC__ << 8) | __GNUC_MINOR__) < ((4 << 8) | 9))
 #define _dmr_thread_local __thread
#endif

/* Macros */
#if defined(_DMR_THREAD_WIN32_)
#define DMR_LOCALS_DTOR_ITERATIONS (4)
#else
#define DMR_LOCALS_DTOR_ITERATIONS PTHREAD_DESTRUCTOR_ITERATIONS
#endif

/* Function return values */
#define dmr_thread_error    0 /**< The requested operation failed */
#define dmr_thread_success  1 /**< The requested operation succeeded */
#define dmr_thread_timedout 2 /**< The time specified in the call was reached without acquiring the requested resource */
#define dmr_thread_busy     3 /**< The requested operation failed because a tesource requested by a test and return function is already in use */
#define dmr_thread_nomem    4 /**< The requested operation failed because it was unable to allocate memory */

/* Mutex types */
#define dmr_mutexplain     0
#define dmr_mutextimed     1
#define dmr_mutexrecursive 2

/* Mutex */
#if defined(_DMR_THREAD_WIN32_)
typedef struct {
  union {
    CRITICAL_SECTION cs;      /* Critical section handle (used for non-timed mutexes) */
    HANDLE mut;               /* Mutex handle (used for timed mutex) */
  } mHandle;                  /* Mutex handle */
  int mAlreadyLocked;         /* TRUE if the mutex is already locked */
  int mRecursive;             /* TRUE if the mutex is recursive */
  int mTimed;                 /* TRUE if the mutex is timed */
} dmr_mutext;
#else
typedef pthread_mutex_t dmr_mutext;
#endif

/** Create a mutex object.
* @param mtx A mutex object.
* @param type Bit-mask that must have one of the following six values:
*   @li @c dmr_mutexplain for a simple non-recursive mutex
*   @li @c dmr_mutextimed for a non-recursive mutex that supports timeout
*   @li @c dmr_mutexplain | @c dmr_mutexrecursive (same as @c dmr_mutexplain, but recursive)
*   @li @c dmr_mutextimed | @c dmr_mutexrecursive (same as @c dmr_mutextimed, but recursive)
* @return @ref dmr_thread_success on success, or @ref dmr_thread_error if the request could
* not be honored.
*/
int dmr_mutexinit(dmr_mutext *mtx, int type);

/** Release any resources used by the given mutex.
* @param mtx A mutex object.
*/
void dmr_mutexdestroy(dmr_mutext *mtx);

/** Lock the given mutex.
* Blocks until the given mutex can be locked. If the mutex is non-recursive, and
* the calling thread already has a lock on the mutex, this call will block
* forever.
* @param mtx A mutex object.
* @return @ref dmr_thread_success on success, or @ref dmr_thread_error if the request could
* not be honored.
*/
int dmr_mutexlock(dmr_mutext *mtx);

/** NOT YET IMPLEMENTED.
*/
int dmr_mutextimedlock(dmr_mutext *mtx, const struct timespec *ts);

/** Try to lock the given mutex.
* The specified mutex shall support either test and return or timeout. If the
* mutex is already locked, the function returns without blocking.
* @param mtx A mutex object.
* @return @ref dmr_thread_success on success, or @ref dmr_thread_busy if the resource
* requested is already in use, or @ref dmr_thread_error if the request could not be
* honored.
*/
int dmr_mutextrylock(dmr_mutext *mtx);

/** Unlock the given mutex.
* @param mtx A mutex object.
* @return @ref dmr_thread_success on success, or @ref dmr_thread_error if the request could
* not be honored.
*/
int dmr_mutexunlock(dmr_mutext *mtx);

/* Condition variable */
#if defined(_DMR_THREAD_WIN32_)
typedef struct {
  HANDLE mEvents[2];                  /* Signal and broadcast event HANDLEs. */
  unsigned int mWaitersCount;         /* Count of the number of waiters. */
  CRITICAL_SECTION mWaitersCountLock; /* Serialize access to mWaitersCount. */
} dmr_cond_t;
#else
typedef pthread_cond_t dmr_cond_t;
#endif

/** Create a condition variable object.
* @param cond A condition variable object.
* @return @ref dmr_thread_success on success, or @ref dmr_thread_error if the request could
* not be honored.
*/
int dmr_cond_init(dmr_cond_t *cond);

/** Release any resources used by the given condition variable.
* @param cond A condition variable object.
*/
void dmr_cond_destroy(dmr_cond_t *cond);

/** Signal a condition variable.
* Unblocks one of the threads that are blocked on the given condition variable
* at the time of the call. If no threads are blocked on the condition variable
* at the time of the call, the function does nothing and return success.
* @param cond A condition variable object.
* @return @ref dmr_thread_success on success, or @ref dmr_thread_error if the request could
* not be honored.
*/
int dmr_cond_signal(dmr_cond_t *cond);

/** Broadcast a condition variable.
* Unblocks all of the threads that are blocked on the given condition variable
* at the time of the call. If no threads are blocked on the condition variable
* at the time of the call, the function does nothing and return success.
* @param cond A condition variable object.
* @return @ref dmr_thread_success on success, or @ref dmr_thread_error if the request could
* not be honored.
*/
int dmr_cond_broadcast(dmr_cond_t *cond);

/** Wait for a condition variable to become signaled.
* The function atomically unlocks the given mutex and endeavors to block until
* the given condition variable is signaled by a call to dmr_cond_signal or to
* dmr_cond_broadcast. When the calling thread becomes unblocked it locks the mutex
* before it returns.
* @param cond A condition variable object.
* @param mtx A mutex object.
* @return @ref dmr_thread_success on success, or @ref dmr_thread_error if the request could
* not be honored.
*/
int dmr_cond_wait(dmr_cond_t *cond, dmr_mutext *mtx);

/** Wait for a condition variable to become signaled.
* The function atomically unlocks the given mutex and endeavors to block until
* the given condition variable is signaled by a call to dmr_cond_signal or to
* dmr_cond_broadcast, or until after the specified time. When the calling thread
* becomes unblocked it locks the mutex before it returns.
* @param cond A condition variable object.
* @param mtx A mutex object.
* @param xt A point in time at which the request will time out (absolute time).
* @return @ref dmr_thread_success upon success, or @ref dmr_thread_timeout if the time
* specified in the call was reached without acquiring the requested resource, or
* @ref dmr_thread_error if the request could not be honored.
*/
int dmr_cond_timedwait(dmr_cond_t *cond, dmr_mutext *mtx, const struct timespec *ts);

/* Thread */
#if defined(_DMR_THREAD_WIN32_)
typedef HANDLE dmr_thread_t;
#else
typedef pthread_t dmr_thread_t;
#endif

/** Thread start function.
* Any thread that is started with the @ref dmr_thread_create() function must be
* started through a function of this type.
* @param arg The thread argument (the @c arg argument of the corresponding
*        @ref dmr_thread_create() call).
* @return The thread return value, which can be obtained by another thread
* by using the @ref dmr_thread_join() function.
*/
typedef int (*dmr_thread_start_t)(void *arg);

/** Create a new thread.
* @param thr Identifier of the newly created thread.
* @param func A function pointer to the function that will be executed in
*        the new thread.
* @param arg An argument to the thread function.
* @return @ref dmr_thread_success on success, or @ref dmr_thread_nomem if no memory could
* be allocated for the thread requested, or @ref dmr_thread_error if the request
* could not be honored.
* @note A thread’s identifier may be reused for a different thread once the
* original thread has exited and either been detached or joined to another
* thread.
*/
int dmr_thread_create(dmr_thread_t *thr, dmr_thread_start_t func, void *arg);

/** Identify the calling thread.
* @return The identifier of the calling thread.
*/
dmr_thread_t dmr_thread_current(void);

/** Dispose of any resources allocated to the thread when that thread exits.
 * @return dmr_thread_success, or dmr_thread_error on error
*/
int dmr_thread_detach(dmr_thread_t thr);

/** Compare two thread identifiers.
* The function determines if two thread identifiers refer to the same thread.
* @return Zero if the two thread identifiers refer to different threads.
* Otherwise a nonzero value is returned.
*/
int dmr_thread_equal(dmr_thread_t thr0, dmr_thread_t thr1);

/** Terminate execution of the calling thread.
* @param res Result code of the calling thread.
*/
DMR_THREAD_NORETURN void dmr_thread_exit(int res);

/** Wait for a thread to terminate.
* The function joins the given thread with the current thread by blocking
* until the other thread has terminated.
* @param thr The thread to join with.
* @param res If this pointer is not NULL, the function will store the result
*        code of the given thread in the integer pointed to by @c res.
* @return @ref dmr_thread_success on success, or @ref dmr_thread_error if the request could
* not be honored.
*/
int dmr_thread_join(dmr_thread_t thr, int *res);

/** Put the calling thread to sleep.
* Suspend execution of the calling thread.
* @param duration  Interval to sleep for
* @param remaining If non-NULL, this parameter will hold the remaining
*                  time until time_point upon return. This will
*                  typically be zero, but if the thread was woken up
*                  by a signal that is not ignored before duration was
*                  reached @c remaining will hold a positive time.
* @return 0 (zero) on successful sleep, -1 if an interrupt occurred,
*         or a negative value if the operation fails.
*/
int dmr_thread_sleep(const struct timespec *duration, struct timespec *remaining);

/** Yield execution to another thread.
* Permit other threads to run, even if the current thread would ordinarily
* continue to run.
*/
void dmr_thread_yield(void);

/* Thread local storage */
#if defined(_DMR_THREAD_WIN32_)
typedef DWORD dmr_locals_t;
#else
typedef pthread_key_t dmr_locals_t;
#endif

/** Destructor function for a thread-specific storage.
* @param val The value of the destructed thread-specific storage.
*/
typedef void (*dmr_locals_dtor_t)(void *val);

/** Create a thread-specific storage.
* @param key The unique key identifier that will be set if the function is
*        successful.
* @param dtor Destructor function. This can be NULL.
* @return @ref dmr_thread_success on success, or @ref dmr_thread_error if the request could
* not be honored.
* @note On Windows, the @c dtor will definitely be called when
* appropriate for threads created with @ref dmr_thread_create.  It will be
* called for other threads in most cases, the possible exception being
* for DLLs loaded with LoadLibraryEx.  In order to be certain, you
* should use @ref dmr_thread_create whenever possible.
*/
int dmr_locals_create(dmr_locals_t *key, dmr_locals_dtor_t dtor);

/** Delete a thread-specific storage.
* The function releases any resources used by the given thread-specific
* storage.
* @param key The key that shall be deleted.
*/
void dmr_locals_delete(dmr_locals_t key);

/** Get the value for a thread-specific storage.
* @param key The thread-specific storage identifier.
* @return The value for the current thread held in the given thread-specific
* storage.
*/
void *dmr_locals_get(dmr_locals_t key);

/** Set the value for a thread-specific storage.
* @param key The thread-specific storage identifier.
* @param val The value of the thread-specific storage to set for the current
*        thread.
* @return @ref dmr_thread_success on success, or @ref dmr_thread_error if the request could
* not be honored.
*/
int dmr_locals_set(dmr_locals_t key, void *val);

#if defined(_DMR_THREAD_WIN32_)
  typedef struct {
    LONG volatile status;
    CRITICAL_SECTION lock;
  } dmr_once_flag;
  #define DMR_ONCE_FLAG_INIT {0,}
#else
  #define dmr_once_flag pthread_once_t
  #define DMR_ONCE_FLAG_INIT PTHREAD_ONCE_INIT
#endif

/** Invoke a callback exactly once
 * @param flag Flag used to ensure the callback is invoked exactly
 *        once.
 * @param func Callback to invoke.
 */
#if defined(_DMR_THREAD_WIN32_)
  void dmr_call_once(dmr_once_flag *flag, void (*func)(void));
#else
  #define dmr_call_once(flag,func) pthread_once(flag,func)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _DMR_THREAD_H */
