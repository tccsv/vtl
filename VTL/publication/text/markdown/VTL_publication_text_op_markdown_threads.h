#ifndef _VTL_PUBLICATION_TEXT_OP_MARKDOWN_THREADS_H
#define _VTL_PUBLICATION_TEXT_OP_MARKDOWN_THREADS_H

// Тонкий shim для потоков и монотонных часов.
// Под Linux/macOS — pthread + clock_gettime(CLOCK_MONOTONIC).
// Под Windows — нативный WinAPI (CreateThread + QueryPerformanceCounter).
// Никаких внешних зависимостей. Локально для markdown-модуля.
//
// Уникальный префикс vtl_md_* — чтобы не конфликтовать с telegram-модулем,
// у которого тот же shim, но под именем vtl_tg_*.

#ifdef _WIN32

#include <windows.h>
#include <stdint.h>

typedef HANDLE vtl_md_thread_t;

// LPTHREAD_START_ROUTINE возвращает DWORD, мы — void*; каст безопасный,
// возврат потока упаковываем в DWORD через GetExitCodeThread.
static inline int vtl_md_thread_create(vtl_md_thread_t* t, void* (*fn)(void*), void* arg)
{
    *t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(void*)fn, arg, 0, NULL);
    return (*t == NULL) ? -1 : 0;
}

static inline void* vtl_md_thread_join(vtl_md_thread_t t)
{
    WaitForSingleObject(t, INFINITE);
    DWORD code = 0;
    GetExitCodeThread(t, &code);
    CloseHandle(t);
    return (void*)(uintptr_t)code;
}

// QueryPerformanceCounter — high-resolution wall-clock, не CPU-time;
// именно он корректен для замера параллельного ускорения
static inline double vtl_md_monotonic_seconds(void)
{
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart / (double)freq.QuadPart;
}

#else

#include <pthread.h>
#include <time.h>

typedef pthread_t vtl_md_thread_t;

static inline int vtl_md_thread_create(vtl_md_thread_t* t, void* (*fn)(void*), void* arg)
{
    return pthread_create(t, NULL, fn, arg);
}

static inline void* vtl_md_thread_join(vtl_md_thread_t t)
{
    void* res = NULL;
    pthread_join(t, &res);
    return res;
}

// CLOCK_MONOTONIC даёт wall-clock — единственный правильный таймер
// для параллельного кода. clock() показывает CPU-time и при N потоках
// выглядел бы как N-кратный рост, а не как ускорение.
static inline double vtl_md_monotonic_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

#endif

#endif
