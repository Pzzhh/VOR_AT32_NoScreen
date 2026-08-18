#ifndef LVGL_LINUX_WINDOWS_COMPAT_H
#define LVGL_LINUX_WINDOWS_COMPAT_H

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <time.h>
#include <SDL2/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t DWORD;
typedef void * LPVOID;
typedef const wchar_t * LPCWSTR;
typedef pthread_t * HANDLE;
typedef DWORD (*LPTHREAD_START_ROUTINE)(LPVOID);

typedef struct {
    uint16_t wYear;
    uint16_t wMonth;
    uint16_t wDayOfWeek;
    uint16_t wDay;
    uint16_t wHour;
    uint16_t wMinute;
    uint16_t wSecond;
    uint16_t wMilliseconds;
} SYSTEMTIME;

#ifndef WINAPI
#define WINAPI
#endif

typedef struct {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;

typedef struct {
    LPTHREAD_START_ROUTINE routine;
    LPVOID argument;
} lv_linux_thread_start_t;

static inline void * lv_linux_thread_trampoline(void * opaque)
{
    lv_linux_thread_start_t start = *(lv_linux_thread_start_t *)opaque;
    free(opaque);
    (void)start.routine(start.argument);
    return NULL;
}

static inline HANDLE CreateThread(void * security, size_t stack_size,
                                  LPTHREAD_START_ROUTINE routine, LPVOID argument,
                                  DWORD flags, DWORD * thread_id)
{
    (void)security;
    (void)stack_size;
    (void)flags;
    HANDLE handle = (HANDLE)malloc(sizeof(*handle));
    lv_linux_thread_start_t * start = (lv_linux_thread_start_t *)malloc(sizeof(*start));
    if(handle == NULL || start == NULL) {
        free(handle);
        free(start);
        return NULL;
    }
    start->routine = routine;
    start->argument = argument;
    if(pthread_create(handle, NULL, lv_linux_thread_trampoline, start) != 0) {
        free(start);
        free(handle);
        return NULL;
    }
    pthread_detach(*handle);
    if(thread_id != NULL) *thread_id = 0;
    return handle;
}

static inline int CloseHandle(HANDLE handle)
{
    free(handle);
    return 1;
}

static inline void ExitThread(DWORD exit_code)
{
    pthread_exit((void *)(uintptr_t)exit_code);
}

static inline void Sleep(DWORD milliseconds) { SDL_Delay(milliseconds); }
static inline DWORD GetTickCount(void) { return SDL_GetTicks(); }
static inline void GetLocalTime(SYSTEMTIME * value)
{
    struct timespec now;
    struct tm local;
    clock_gettime(CLOCK_REALTIME, &now);
    localtime_r(&now.tv_sec, &local);
    value->wYear = (uint16_t)(local.tm_year + 1900);
    value->wMonth = (uint16_t)(local.tm_mon + 1);
    value->wDayOfWeek = (uint16_t)local.tm_wday;
    value->wDay = (uint16_t)local.tm_mday;
    value->wHour = (uint16_t)local.tm_hour;
    value->wMinute = (uint16_t)local.tm_min;
    value->wSecond = (uint16_t)local.tm_sec;
    value->wMilliseconds = (uint16_t)(now.tv_nsec / 1000000L);
}
static inline void OutputDebugStringA(const char * text)
{
    if(text != NULL) fputs(text, stderr);
}

#ifdef __cplusplus
}
#endif

#endif
