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

#include <stdlib.h>
#include "dmr/platform.h"
#include "dmr/thread.h"

/* Platform specific includes */
#if defined(_DMR_THREAD_POSIX_)
  #include <signal.h>
  #include <sched.h>
  #include <unistd.h>
  #include <sys/time.h>
  #include <errno.h>
#elif defined(_DMR_THREAD_WIN32_)
  #include <process.h>
  #include <sys/timeb.h>
#endif

/* Standard, good-to-have defines */
#ifndef NULL
  #define NULL (void*)0
#endif
#ifndef TRUE
  #define TRUE 1
#endif
#ifndef FALSE
  #define FALSE 0
#endif

#ifdef __cplusplus
extern "C" {
#endif


int dmr_mutexinit(dmr_mutext *mtx, int type)
{
#if defined(_DMR_THREAD_WIN32_)
  mtx->mAlreadyLocked = FALSE;
  mtx->mRecursive = type & dmr_mutexrecursive;
  mtx->mTimed = type & dmr_mutextimed;
  if (!mtx->mTimed)
  {
    InitializeCriticalSection(&(mtx->mHandle.cs));
  }
  else
  {
    mtx->mHandle.mut = CreateMutex(NULL, FALSE, NULL);
    if (mtx->mHandle.mut == NULL)
    {
      return dmr_thread_error;
    }
  }
  return dmr_thread_success;
#else
  int ret;
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  if (type & dmr_mutexrecursive)
  {
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  }
  ret = pthread_mutex_init(mtx, &attr);
  pthread_mutexattr_destroy(&attr);
  return ret == 0 ? dmr_thread_success : dmr_thread_error;
#endif
}

void dmr_mutexdestroy(dmr_mutext *mtx)
{
#if defined(_DMR_THREAD_WIN32_)
  if (!mtx->mTimed)
  {
    DeleteCriticalSection(&(mtx->mHandle.cs));
  }
  else
  {
    CloseHandle(mtx->mHandle.mut);
  }
#else
  pthread_mutex_destroy(mtx);
#endif
}

int dmr_mutexlock(dmr_mutext *mtx)
{
#if defined(_DMR_THREAD_WIN32_)
  if (!mtx->mTimed)
  {
    EnterCriticalSection(&(mtx->mHandle.cs));
  }
  else
  {
    switch (WaitForSingleObject(mtx->mHandle.mut, INFINITE))
    {
      case WAIT_OBJECT_0:
        break;
      case WAIT_ABANDONED:
      default:
        return dmr_thread_error;
    }
  }

  if (!mtx->mRecursive)
  {
    while(mtx->mAlreadyLocked) Sleep(1); /* Simulate deadlock... */
    mtx->mAlreadyLocked = TRUE;
  }
  return dmr_thread_success;
#else
  return pthread_mutex_lock(mtx) == 0 ? dmr_thread_success : dmr_thread_error;
#endif
}

int dmr_mutextimedlock(dmr_mutext *mtx, const struct timespec *ts)
{
#if defined(_DMR_THREAD_WIN32_)
  struct timespec current_ts;
  DWORD timeoutMs;

  if (!mtx->mTimed)
  {
    return dmr_thread_error;
  }

  timespec_get(&current_ts, TIME_UTC);

  if ((current_ts.tv_sec > ts->tv_sec) || ((current_ts.tv_sec == ts->tv_sec) && (current_ts.tv_nsec >= ts->tv_nsec)))
  {
    timeoutMs = 0;
  }
  else
  {
    timeoutMs  = (DWORD)(ts->tv_sec  - current_ts.tv_sec)  * 1000;
    timeoutMs += (ts->tv_nsec - current_ts.tv_nsec) / 1000000;
    timeoutMs += 1;
  }

  /* TODO: the timeout for WaitForSingleObject doesn't include time
     while the computer is asleep. */
  switch (WaitForSingleObject(mtx->mHandle.mut, timeoutMs))
  {
    case WAIT_OBJECT_0:
      break;
    case WAIT_TIMEOUT:
      return dmr_thread_timedout;
    case WAIT_ABANDONED:
    default:
      return dmr_thread_error;
  }

  if (!mtx->mRecursive)
  {
    while(mtx->mAlreadyLocked) Sleep(1); /* Simulate deadlock... */
    mtx->mAlreadyLocked = TRUE;
  }

  return dmr_thread_success;
#elif defined(_POSIX_TIMEOUTS) && (_POSIX_TIMEOUTS >= 200112L) && defined(_POSIX_THREADS) && (_POSIX_THREADS >= 200112L)
  switch (pthread_mutex_timedlock(mtx, ts)) {
    case 0:
      return dmr_thread_success;
    case ETIMEDOUT:
      return dmr_thread_timedout;
    default:
      return dmr_thread_error;
  }
#else
  int rc;
  struct timespec cur, dur;

  /* Try to acquire the lock and, if we fail, sleep for 5ms. */
  while ((rc = pthread_mutex_trylock (mtx)) == EBUSY) {
    timespec_get(&cur, TIME_UTC);

    if ((cur.tv_sec > ts->tv_sec) || ((cur.tv_sec == ts->tv_sec) && (cur.tv_nsec >= ts->tv_nsec)))
    {
      break;
    }

    dur.tv_sec = ts->tv_sec - cur.tv_sec;
    dur.tv_nsec = ts->tv_nsec - cur.tv_nsec;
    if (dur.tv_nsec < 0)
    {
      dur.tv_sec--;
      dur.tv_nsec += 1000000000;
    }

    if ((dur.tv_sec != 0) || (dur.tv_nsec > 5000000))
    {
      dur.tv_sec = 0;
      dur.tv_nsec = 5000000;
    }

    nanosleep(&dur, NULL);
  }

  switch (rc) {
    case 0:
      return dmr_thread_success;
    case ETIMEDOUT:
    case EBUSY:
      return dmr_thread_timedout;
    default:
      return dmr_thread_error;
  }
#endif
}

int dmr_mutextrylock(dmr_mutext *mtx)
{
#if defined(_DMR_THREAD_WIN32_)
  int ret;

  if (!mtx->mTimed)
  {
    ret = TryEnterCriticalSection(&(mtx->mHandle.cs)) ? dmr_thread_success : dmr_thread_busy;
  }
  else
  {
    ret = (WaitForSingleObject(mtx->mHandle.mut, 0) == WAIT_OBJECT_0) ? dmr_thread_success : dmr_thread_busy;
  }

  if ((!mtx->mRecursive) && (ret == dmr_thread_success))
  {
    if (mtx->mAlreadyLocked)
    {
      LeaveCriticalSection(&(mtx->mHandle.cs));
      ret = dmr_thread_busy;
    }
    else
    {
      mtx->mAlreadyLocked = TRUE;
    }
  }
  return ret;
#else
  return (pthread_mutex_trylock(mtx) == 0) ? dmr_thread_success : dmr_thread_busy;
#endif
}

int dmr_mutexunlock(dmr_mutext *mtx)
{
#if defined(_DMR_THREAD_WIN32_)
  mtx->mAlreadyLocked = FALSE;
  if (!mtx->mTimed)
  {
    LeaveCriticalSection(&(mtx->mHandle.cs));
  }
  else
  {
    if (!ReleaseMutex(mtx->mHandle.mut))
    {
      return dmr_thread_error;
    }
  }
  return dmr_thread_success;
#else
  return pthread_mutex_unlock(mtx) == 0 ? dmr_thread_success : dmr_thread_error;;
#endif
}

#if defined(_DMR_THREAD_WIN32_)
#define _CONDITION_EVENT_ONE 0
#define _CONDITION_EVENT_ALL 1
#endif

int dmr_cond_init(dmr_cond_t *cond)
{
#if defined(_DMR_THREAD_WIN32_)
  cond->mWaitersCount = 0;

  /* Init critical section */
  InitializeCriticalSection(&cond->mWaitersCountLock);

  /* Init events */
  cond->mEvents[_CONDITION_EVENT_ONE] = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (cond->mEvents[_CONDITION_EVENT_ONE] == NULL)
  {
    cond->mEvents[_CONDITION_EVENT_ALL] = NULL;
    return dmr_thread_error;
  }
  cond->mEvents[_CONDITION_EVENT_ALL] = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (cond->mEvents[_CONDITION_EVENT_ALL] == NULL)
  {
    CloseHandle(cond->mEvents[_CONDITION_EVENT_ONE]);
    cond->mEvents[_CONDITION_EVENT_ONE] = NULL;
    return dmr_thread_error;
  }

  return dmr_thread_success;
#else
  return pthread_cond_init(cond, NULL) == 0 ? dmr_thread_success : dmr_thread_error;
#endif
}

void dmr_cond_destroy(dmr_cond_t *cond)
{
#if defined(_DMR_THREAD_WIN32_)
  if (cond->mEvents[_CONDITION_EVENT_ONE] != NULL)
  {
    CloseHandle(cond->mEvents[_CONDITION_EVENT_ONE]);
  }
  if (cond->mEvents[_CONDITION_EVENT_ALL] != NULL)
  {
    CloseHandle(cond->mEvents[_CONDITION_EVENT_ALL]);
  }
  DeleteCriticalSection(&cond->mWaitersCountLock);
#else
  pthread_cond_destroy(cond);
#endif
}

int dmr_cond_signal(dmr_cond_t *cond)
{
#if defined(_DMR_THREAD_WIN32_)
  int haveWaiters;

  /* Are there any waiters? */
  EnterCriticalSection(&cond->mWaitersCountLock);
  haveWaiters = (cond->mWaitersCount > 0);
  LeaveCriticalSection(&cond->mWaitersCountLock);

  /* If we have any waiting threads, send them a signal */
  if(haveWaiters)
  {
    if (SetEvent(cond->mEvents[_CONDITION_EVENT_ONE]) == 0)
    {
      return dmr_thread_error;
    }
  }

  return dmr_thread_success;
#else
  return pthread_cond_signal(cond) == 0 ? dmr_thread_success : dmr_thread_error;
#endif
}

int dmr_cond_broadcast(dmr_cond_t *cond)
{
#if defined(_DMR_THREAD_WIN32_)
  int haveWaiters;

  /* Are there any waiters? */
  EnterCriticalSection(&cond->mWaitersCountLock);
  haveWaiters = (cond->mWaitersCount > 0);
  LeaveCriticalSection(&cond->mWaitersCountLock);

  /* If we have any waiting threads, send them a signal */
  if(haveWaiters)
  {
    if (SetEvent(cond->mEvents[_CONDITION_EVENT_ALL]) == 0)
    {
      return dmr_thread_error;
    }
  }

  return dmr_thread_success;
#else
  return pthread_cond_broadcast(cond) == 0 ? dmr_thread_success : dmr_thread_error;
#endif
}

#if defined(_DMR_THREAD_WIN32_)
static int _dmr_cond_timedwait_win32(dmr_cond_t *cond, dmr_mutext *mtx, DWORD timeout)
{
  int result, lastWaiter;

  /* Increment number of waiters */
  EnterCriticalSection(&cond->mWaitersCountLock);
  ++ cond->mWaitersCount;
  LeaveCriticalSection(&cond->mWaitersCountLock);

  /* Release the mutex while waiting for the condition (will decrease
     the number of waiters when done)... */
  dmr_mutexunlock(mtx);

  /* Wait for either event to become signaled due to dmr_cond_signal() or
     dmr_cond_broadcast() being called */
  result = WaitForMultipleObjects(2, cond->mEvents, FALSE, timeout);
  if (result == WAIT_TIMEOUT)
  {
    /* The mutex is locked again before the function returns, even if an error occurred */
    dmr_mutexlock(mtx);
    return dmr_thread_timedout;
  }
  else if (result == (int)WAIT_FAILED)
  {
    /* The mutex is locked again before the function returns, even if an error occurred */
    dmr_mutexlock(mtx);
    return dmr_thread_error;
  }

  /* Check if we are the last waiter */
  EnterCriticalSection(&cond->mWaitersCountLock);
  -- cond->mWaitersCount;
  lastWaiter = (result == (WAIT_OBJECT_0 + _CONDITION_EVENT_ALL)) &&
               (cond->mWaitersCount == 0);
  LeaveCriticalSection(&cond->mWaitersCountLock);

  /* If we are the last waiter to be notified to stop waiting, reset the event */
  if (lastWaiter)
  {
    if (ResetEvent(cond->mEvents[_CONDITION_EVENT_ALL]) == 0)
    {
      /* The mutex is locked again before the function returns, even if an error occurred */
      dmr_mutexlock(mtx);
      return dmr_thread_error;
    }
  }

  /* Re-acquire the mutex */
  dmr_mutexlock(mtx);

  return dmr_thread_success;
}
#endif

int dmr_cond_wait(dmr_cond_t *cond, dmr_mutext *mtx)
{
#if defined(_DMR_THREAD_WIN32_)
  return _dmr_cond_timedwait_win32(cond, mtx, INFINITE);
#else
  return pthread_cond_wait(cond, mtx) == 0 ? dmr_thread_success : dmr_thread_error;
#endif
}

int dmr_cond_timedwait(dmr_cond_t *cond, dmr_mutext *mtx, const struct timespec *ts)
{
#if defined(_DMR_THREAD_WIN32_)
  struct timespec now;
  if (timespec_get(&now, TIME_UTC) == TIME_UTC)
  {
    unsigned long long nowInMilliseconds = now.tv_sec * 1000 + now.tv_nsec / 1000000;
    unsigned long long tsInMilliseconds  = ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
    DWORD delta = (tsInMilliseconds > nowInMilliseconds) ?
      (DWORD)(tsInMilliseconds - nowInMilliseconds) : 0;
    return _dmr_cond_timedwait_win32(cond, mtx, delta);
  }
  else
    return dmr_thread_error;
#else
  int ret;
  ret = pthread_cond_timedwait(cond, mtx, ts);
  if (ret == ETIMEDOUT)
  {
    return dmr_thread_timedout;
  }
  return ret == 0 ? dmr_thread_success : dmr_thread_error;
#endif
}

#if defined(_DMR_THREAD_WIN32_)
struct dmr_thread_tss_data {
  void* value;
  dmr_locals_t key;
  struct dmr_thread_tss_data* next;
};

static dmr_locals_dtor_t _dmr_thread_locals_dtors[1088] = { NULL, };

static _dmr_thread_local struct dmr_thread_tss_data* _dmr_thread_locals_head = NULL;
static _dmr_thread_local struct dmr_thread_tss_data* _dmr_thread_locals_tail = NULL;

static void _dmr_thread_locals_cleanup (void);

static void _dmr_thread_locals_cleanup (void) {
  struct dmr_thread_tss_data* data;
  int iteration;
  unsigned int again = 1;
  void* value;

  for (iteration = 0 ; iteration < DMR_LOCALS_DTOR_ITERATIONS && again > 0 ; iteration++)
  {
    again = 0;
    for (data = _dmr_thread_locals_head ; data != NULL ; data = data->next)
    {
      if (data->value != NULL)
      {
        value = data->value;
        data->value = NULL;

        if (_dmr_thread_locals_dtors[data->key] != NULL)
        {
          again = 1;
          _dmr_thread_locals_dtors[data->key](value);
        }
      }
    }
  }

  while (_dmr_thread_locals_head != NULL) {
    data = _dmr_thread_locals_head->next;
    free (_dmr_thread_locals_head);
    _dmr_thread_locals_head = data;
  }
  _dmr_thread_locals_head = NULL;
  _dmr_thread_locals_tail = NULL;
}

static void NTAPI _dmr_thread_locals_callback(PVOID h, DWORD dwReason, PVOID pv)
{
  (void)h;
  (void)pv;

  if (_dmr_thread_locals_head != NULL && (dwReason == DLL_THREAD_DETACH || dwReason == DLL_PROCESS_DETACH))
  {
    _dmr_thread_locals_cleanup();
  }
}

#if defined(_MSC_VER)
  #ifdef _M_X64
    #pragma const_seg(".CRT$XLB")
  #else
    #pragma data_seg(".CRT$XLB")
  #endif
  PIMAGE_TLS_CALLBACK p_thread_callback = _dmr_thread_locals_callback;
  #ifdef _M_X64
    #pragma data_seg()
  #else
    #pragma const_seg()
  #endif
#else
  PIMAGE_TLS_CALLBACK p_thread_callback __attribute__((section(".CRT$XLB"))) = _dmr_thread_locals_callback;
#endif

#endif /* defined(_DMR_THREAD_WIN32_) */

/** Information to pass to the new thread (what to run). */
typedef struct {
  dmr_thread_start_t mFunction; /**< Pointer to the function to be executed. */
  void * mArg;            /**< Function argument for the thread function. */
} _thread_start_info;

/* Thread wrapper function. */
#if defined(_DMR_THREAD_WIN32_)
static DWORD WINAPI _dmr_thread_wrapper_function(LPVOID aArg)
#elif defined(_DMR_THREAD_POSIX_)
static void * _dmr_thread_wrapper_function(void * aArg)
#endif
{
  dmr_thread_start_t fun;
  void *arg;
  int  res;

  /* Get thread startup information */
  _thread_start_info *ti = (_thread_start_info *) aArg;
  fun = ti->mFunction;
  arg = ti->mArg;

  /* The thread is responsible for freeing the startup information */
  free((void *)ti);

  /* Call the actual client thread function */
  res = fun(arg);

#if defined(_DMR_THREAD_WIN32_)
  if (_dmr_thread_locals_head != NULL)
  {
    _dmr_thread_locals_cleanup();
  }

  return (DWORD)res;
#else
  return (void*)(intptr_t)res;
#endif
}

int dmr_thread_create(dmr_thread_t *thr, dmr_thread_start_t func, void *arg)
{
  /* Fill out the thread startup information (passed to the thread wrapper,
     which will eventually free it) */
  _thread_start_info* ti = (_thread_start_info*)malloc(sizeof(_thread_start_info));
  if (ti == NULL)
  {
    return dmr_thread_nomem;
  }
  ti->mFunction = func;
  ti->mArg = arg;

  /* Create the thread */
#if defined(_DMR_THREAD_WIN32_)
  *thr = CreateThread(NULL, 0, _dmr_thread_wrapper_function, (LPVOID) ti, 0, NULL);
#elif defined(_DMR_THREAD_POSIX_)
  if(pthread_create(thr, NULL, _dmr_thread_wrapper_function, (void *)ti) != 0)
  {
    *thr = 0;
  }
#endif

  /* Did we fail to create the thread? */
  if(!*thr)
  {
    free(ti);
    return dmr_thread_error;
  }

  return dmr_thread_success;
}

dmr_thread_t dmr_thread_current(void)
{
#if defined(_DMR_THREAD_WIN32_)
  return GetCurrentThread();
#else
  return pthread_self();
#endif
}

int dmr_thread_detach(dmr_thread_t thr)
{
#if defined(_DMR_THREAD_WIN32_)
  /* https://stackoverflow.com/questions/12744324/how-to-detach-a-thread-on-windows-c#answer-12746081 */
  return CloseHandle(thr) != 0 ? dmr_thread_success : dmr_thread_error;
#else
  return pthread_detach(thr) == 0 ? dmr_thread_success : dmr_thread_error;
#endif
}

int dmr_thread_equal(dmr_thread_t thr0, dmr_thread_t thr1)
{
#if defined(_DMR_THREAD_WIN32_)
  return thr0 == thr1;
#else
  return pthread_equal(thr0, thr1);
#endif
}

void dmr_thread_exit(int res)
{
#if defined(_DMR_THREAD_WIN32_)
  if (_dmr_thread_locals_head != NULL)
  {
    _dmr_thread_locals_cleanup();
  }

  ExitThread(res);
#else
  pthread_exit((void*)(intptr_t)res);
#endif
}

int dmr_thread_join(dmr_thread_t thr, int *res)
{
#if defined(_DMR_THREAD_WIN32_)
  DWORD dwRes;

  if (WaitForSingleObject(thr, INFINITE) == WAIT_FAILED)
  {
    return dmr_thread_error;
  }
  if (res != NULL)
  {
    if (GetExitCodeThread(thr, &dwRes) != 0)
    {
      *res = dwRes;
    }
    else
    {
      return dmr_thread_error;
    }
  }
  CloseHandle(thr);
#elif defined(_DMR_THREAD_POSIX_)
  void *pres;
  if (pthread_join(thr, &pres) != 0)
  {
    return dmr_thread_error;
  }
  if (res != NULL)
  {
    *res = (int)(intptr_t)pres;
  }
#endif
  return dmr_thread_success;
}

int dmr_thread_sleep(const struct timespec *duration, struct timespec *remaining)
{
#if !defined(_DMR_THREAD_WIN32_)
  return nanosleep(duration, remaining);
#else
  struct timespec start;
  DWORD t;

  timespec_get(&start, TIME_UTC);

  t = SleepEx((DWORD)(duration->tv_sec * 1000 +
              duration->tv_nsec / 1000000 +
              (((duration->tv_nsec % 1000000) == 0) ? 0 : 1)),
              TRUE);

  if (t == 0) {
    return 0;
  } else if (remaining != NULL) {
    timespec_get(remaining, TIME_UTC);
    remaining->tv_sec -= start.tv_sec;
    remaining->tv_nsec -= start.tv_nsec;
    if (remaining->tv_nsec < 0)
    {
      remaining->tv_nsec += 1000000000;
      remaining->tv_sec -= 1;
    }
  } else {
    return -1;
  }

  return 0;
#endif
}

void dmr_thread_yield(void)
{
#if defined(_DMR_THREAD_WIN32_)
  Sleep(0);
#else
  sched_yield();
#endif
}

int dmr_locals_create(dmr_locals_t *key, dmr_locals_dtor_t dtor)
{
#if defined(_DMR_THREAD_WIN32_)
  *key = TlsAlloc();
  if (*key == TLS_OUT_OF_INDEXES)
  {
    return dmr_thread_error;
  }
  _dmr_thread_locals_dtors[*key] = dtor;
#else
  if (pthread_key_create(key, dtor) != 0)
  {
    return dmr_thread_error;
  }
#endif
  return dmr_thread_success;
}

void dmr_locals_delete(dmr_locals_t key)
{
#if defined(_DMR_THREAD_WIN32_)
  struct dmr_thread_tss_data* data = (struct dmr_thread_tss_data*) TlsGetValue (key);
  struct dmr_thread_tss_data* prev = NULL;
  if (data != NULL)
  {
    if (data == _dmr_thread_locals_head)
    {
      _dmr_thread_locals_head = data->next;
    }
    else
    {
      prev = _dmr_thread_locals_head;
      if (prev != NULL)
      {
        while (prev->next != data)
        {
          prev = prev->next;
        }
      }
    }

    if (data == _dmr_thread_locals_tail)
    {
      _dmr_thread_locals_tail = prev;
    }

    free (data);
  }
  _dmr_thread_locals_dtors[key] = NULL;
  TlsFree(key);
#else
  pthread_key_delete(key);
#endif
}

void *dmr_locals_get(dmr_locals_t key)
{
#if defined(_DMR_THREAD_WIN32_)
  struct dmr_thread_tss_data* data = (struct dmr_thread_tss_data*)TlsGetValue(key);
  if (data == NULL)
  {
    return NULL;
  }
  return data->value;
#else
  return pthread_getspecific(key);
#endif
}

int dmr_locals_set(dmr_locals_t key, void *val)
{
#if defined(_DMR_THREAD_WIN32_)
  struct dmr_thread_tss_data* data = (struct dmr_thread_tss_data*)TlsGetValue(key);
  if (data == NULL)
  {
    data = (struct dmr_thread_tss_data*)malloc(sizeof(struct dmr_thread_tss_data));
    if (data == NULL)
    {
      return dmr_thread_error;
	}

    data->value = NULL;
    data->key = key;
    data->next = NULL;

    if (_dmr_thread_locals_tail != NULL)
    {
      _dmr_thread_locals_tail->next = data;
    }
    else
    {
      _dmr_thread_locals_tail = data;
    }

    if (_dmr_thread_locals_head == NULL)
    {
      _dmr_thread_locals_head = data;
    }

    if (!TlsSetValue(key, data))
    {
      free (data);
	  return dmr_thread_error;
    }
  }
  data->value = val;
#else
  if (pthread_setspecific(key, val) != 0)
  {
    return dmr_thread_error;
  }
#endif
  return dmr_thread_success;
}

#if defined(_DMR_THREAD_EMULATE_TIMESPEC_GET_)
int _dmr_thread_timespec_get(struct timespec *ts, int base)
{
#if defined(_DMR_THREAD_WIN32_)
  struct _timeb tb;
#elif !defined(CLOCK_REALTIME)
  struct timeval tv;
#endif

  if (base != TIME_UTC)
  {
    return 0;
  }

#if defined(_DMR_THREAD_WIN32_)
  _ftime_s(&tb);
  ts->tv_sec = (time_t)tb.time;
  ts->tv_nsec = 1000000L * (long)tb.millitm;
#elif defined(CLOCK_REALTIME)
  base = (clock_gettime(CLOCK_REALTIME, ts) == 0) ? base : 0;
#else
  gettimeofday(&tv, NULL);
  ts->tv_sec = (time_t)tv.tv_sec;
  ts->tv_nsec = 1000L * (long)tv.tv_usec;
#endif

  return base;
}
#endif /* _DMR_THREAD_EMULATE_TIMESPEC_GET_ */

#if defined(_DMR_THREAD_WIN32_)
void dmr_call_once(dmr_once_flag *flag, void (*func)(void))
{
  /* The idea here is that we use a spin lock (via the
     InterlockedCompareExchange function) to restrict access to the
     critical section until we have initialized it, then we use the
     critical section to block until the callback has completed
     execution. */
  while (flag->status < 3)
  {
    switch (flag->status)
    {
      case 0:
        if (InterlockedCompareExchange (&(flag->status), 1, 0) == 0) {
          InitializeCriticalSection(&(flag->lock));
          EnterCriticalSection(&(flag->lock));
          flag->status = 2;
          func();
          flag->status = 3;
          LeaveCriticalSection(&(flag->lock));
          return;
        }
        break;
      case 1:
        break;
      case 2:
        EnterCriticalSection(&(flag->lock));
        LeaveCriticalSection(&(flag->lock));
        break;
    }
  }
}
#endif /* defined(_DMR_THREAD_WIN32_) */

#ifdef __cplusplus
}
#endif
