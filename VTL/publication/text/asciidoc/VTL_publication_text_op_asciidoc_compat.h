#ifndef _VTL_PUBLICATION_TEXT_OP_ASCIIDOC_COMPAT_H
#define _VTL_PUBLICATION_TEXT_OP_ASCIIDOC_COMPAT_H

// Тонкий слой совместимости — потоки и монотонные часы.
// Под Linux/macOS используем pthread + clock_gettime,
// под Windows — нативное WinAPI (CreateThread + QueryPerformanceCounter).
// Никаких внешних зависимостей.

#ifdef _WIN32

#include <windows.h>
#include <stdint.h>

typedef HANDLE vtl_thread_t;

static inline int vtl_thread_create(vtl_thread_t* t, void* (*fn)(void*), void* arg)
{
    *t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(void*)fn, arg, 0, NULL);
    return (*t == NULL) ? -1 : 0;
}

static inline void* vtl_thread_join(vtl_thread_t t)
{
    WaitForSingleObject(t, INFINITE);
    DWORD code = 0;
    GetExitCodeThread(t, &code);
    CloseHandle(t);
    return (void*)(uintptr_t)code;
}

static inline double vtl_monotonic_seconds(void)
{
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart / (double)freq.QuadPart;
}

#else

#include <pthread.h>
#include <time.h>

typedef pthread_t vtl_thread_t;

static inline int vtl_thread_create(vtl_thread_t* t, void* (*fn)(void*), void* arg)
{
    return pthread_create(t, NULL, fn, arg);
}

static inline void* vtl_thread_join(vtl_thread_t t)
{
    void* res = NULL;
    pthread_join(t, &res);
    return res;
}

static inline double vtl_monotonic_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

#endif

#endif
