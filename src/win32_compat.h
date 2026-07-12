#ifndef KALLISTO_WIN32_COMPAT_H
#define KALLISTO_WIN32_COMPAT_H

#ifdef _MSC_VER

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <sys/stat.h>
#include <direct.h>
#include <io.h>
#include <intrin.h>
#include <stdint.h>

// MSVC's <sys/stat.h> provides _stat64 but may not define 'struct stat'.
// This macro maps both the function name and the struct name.
#if defined(_MSC_VER) && !defined(stat)
#define stat _stat64
#endif

// Windows headers may define 'byte' (unsigned char), which conflicts
// with C++17's std::byte. Undefine it since this codebase uses neither.
#ifdef byte
#undef byte
#endif

#ifdef _WIN64
typedef unsigned int uint;
#endif

typedef unsigned short mode_t;

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & _S_IFDIR) != 0)
#endif

#define fileno _fileno

// -- timeval and gettimeofday -------------------------------------------

struct timeval {
    long tv_sec;
    long tv_usec;
};

inline int gettimeofday(struct timeval* tv, void* tz) {
    (void)tz;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    li.QuadPart -= 116444736000000000ULL;
    tv->tv_sec = (long)(li.QuadPart / 10000000);
    tv->tv_usec = (long)((li.QuadPart % 10000000) / 10);
    return 0;
}

// -- GCC __builtin_ffsll -> MSVC _BitScanForward64 -----------------------
// ffsll: find first set bit (1-based), 0 if x==0.
// Used as ffsll(pow2) - 1 = log2(pow2).
inline int win32_ffsll(unsigned long long x) {
    if (x == 0) return 0;
    unsigned long idx;
    _BitScanForward64(&idx, x);
    return (int)(idx + 1);
}
#define __builtin_ffsll(x) win32_ffsll(x)

// -- GCC __sync_fetch_and_add -> MSVC InterlockedExchangeAdd ------------

template<typename TP, typename TV>
inline TP win32_atomic_fetch_add(TP volatile* ptr, TV val) {
    if (sizeof(TP) == 8) {
        return (TP)InterlockedExchangeAdd64((LONG64 volatile*)ptr, (LONG64)(TP)val);
    } else {
        return (TP)InterlockedExchangeAdd((LONG volatile*)ptr, (LONG)(TP)val);
    }
}
#define __sync_fetch_and_add(ptr, val) win32_atomic_fetch_add((ptr), (val))

// -- GCC __sync_fetch_and_or / __sync_fetch_and_and -> MSVC InterlockedOr64/And64

inline uint64_t win32_atomic_fetch_or(uint64_t volatile* ptr, uint64_t val) {
    return InterlockedOr64((LONG64 volatile*)ptr, (LONG64)val);
}
#define __sync_fetch_and_or(ptr, val) win32_atomic_fetch_or((ptr), (val))

inline uint64_t win32_atomic_fetch_and(uint64_t volatile* ptr, uint64_t val) {
    return InterlockedAnd64((LONG64 volatile*)ptr, (LONG64)val);
}
#define __sync_fetch_and_and(ptr, val) win32_atomic_fetch_and((ptr), (val))

// -- pthread_mutex_t -> SRWLOCK ----------------------------------------

typedef SRWLOCK pthread_mutex_t;

inline int win32_pthread_mutex_init(pthread_mutex_t* m, void*) {
    InitializeSRWLock(m);
    return 0;
}
#define pthread_mutex_init(m, a) win32_pthread_mutex_init((m), (a))

inline int win32_pthread_mutex_destroy(pthread_mutex_t*) {
    return 0;
}
#define pthread_mutex_destroy(m) win32_pthread_mutex_destroy(m)

inline int win32_pthread_mutex_lock(pthread_mutex_t* m) {
    AcquireSRWLockExclusive(m);
    return 0;
}
#define pthread_mutex_lock(m) win32_pthread_mutex_lock(m)

inline int win32_pthread_mutex_unlock(pthread_mutex_t* m) {
    ReleaseSRWLockExclusive(m);
    return 0;
}
#define pthread_mutex_unlock(m) win32_pthread_mutex_unlock(m)

// -- pthread_t -> HANDLE, pthread_create/join --------------------------
// pthread_create CANNOT be replaced via a C preprocessor macro because
// it is called with C++ template function arguments that contain commas
// (e.g. thread_processLevel<A, B, C>). The individual call sites in
// BooPHF.h and other files must be guarded with #ifdef _WIN32 and use
// the win32_thread_* helpers below directly.
//
// pthread_join is safe to macro-ize since it takes simple arguments.

typedef HANDLE pthread_t;

struct win32_thread_wrapper {
    void* (*fn)(void*);
    void* arg;
};

inline DWORD WINAPI win32_thread_start(LPVOID param) {
    win32_thread_wrapper* w = (win32_thread_wrapper*)param;
    void* (*fn)(void*) = w->fn;
    void* arg = w->arg;
    delete w;
    fn(arg);
    return 0;
}


inline int win32_pthread_join(pthread_t thread, void**) {
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return 0;
}
#define pthread_join(t, r) win32_pthread_join((t), (r))

#endif // _MSC_VER
#endif // KALLISTO_WIN32_COMPAT_H
