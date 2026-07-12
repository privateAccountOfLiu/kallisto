#ifndef HTSLIB_WIN32_COMPAT_H
#define HTSLIB_WIN32_COMPAT_H

#ifdef _MSC_VER

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <sys/stat.h>
#include <io.h>
#include <direct.h>
#include <stdint.h>
#include <process.h>

// MSVC's <windows.h> defines ERROR, which conflicts with htslib enums
#ifdef ERROR
#undef ERROR
#endif

#ifdef __cplusplus
extern "C" {
#endif

// =====================================================================
// POSIX → MSVC replacements (for htslib C code)
// =====================================================================

// -- stat -------------------------------------------------------------
#define stat _stat64
#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & _S_IFDIR) != 0)
#endif

// -- unistd.h stubs ---------------------------------------------------
#define R_OK 4
#define W_OK 2
#define X_OK 0
#define F_OK 0
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

// -- mkdir with mode_t (POSIX has 2 args, MSVC has 1) ------------------
static inline int win32_mkdir(const char* path, int mode) {
    (void)mode;
    return _mkdir(path);
}
#define mkdir(path, mode) win32_mkdir((path), (mode))

// -- file I/O ---------------------------------------------------------
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_RDWR   _O_RDWR
#define O_CREAT  _O_CREAT
#define O_TRUNC  _O_TRUNC
#define O_APPEND _O_APPEND
#define O_BINARY _O_BINARY
#define O_EXCL   _O_EXCL

// -- usleep -----------------------------------------------------------
#define usleep(us) Sleep(((us) + 999) / 1000)

// -- process/fs --------------------------------------------------------
#define getpid()   _getpid()
#define getcwd(b,s) _getcwd((b),(s))
#define sysconf(x)  ((x) == _SC_PAGESIZE ? 4096 : 512)
#undef  _SC_PAGESIZE
#define _SC_PAGESIZE 47

// -- off_t (MSVC doesn't define it; typedef from _off_t in sys/types.h) -
#ifndef _OFF_T_DEFINED
typedef _off_t off_t;
#define _OFF_T_DEFINED
#endif

// -- PATH_MAX ---------------------------------------------------------
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// -- strcasecmp / strncasecmp -----------------------------------------
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp

// -- ssize_t -----------------------------------------------------------
#ifndef _SSIZE_T_DEFINED
typedef SSIZE_T ssize_t;
#define _SSIZE_T_DEFINED
#endif

// -- DIR / opendir / readdir / closedir → FindFirstFile/FindNextFile ----
// (Only used in plugin.c; minimal implementation)

struct dirent {
    char d_name[260]; // MAX_PATH
};

typedef struct {
    HANDLE hFind;
    WIN32_FIND_DATAA findData;
    int first;
    struct dirent ent;
} DIR;

static inline DIR* opendir(const char* path) {
    char pattern[300];
    int len = (int)strlen(path);
    if (len + 3 > (int)sizeof(pattern)) return NULL;
    memcpy(pattern, path, len);
    if (len > 0 && pattern[len-1] != '/' && pattern[len-1] != '\\') {
        pattern[len++] = '\\';
    }
    pattern[len++] = '*';
    pattern[len] = '\0';

    DIR* d = (DIR*)calloc(1, sizeof(DIR));
    if (!d) return NULL;
    d->hFind = FindFirstFileA(pattern, &d->findData);
    if (d->hFind == INVALID_HANDLE_VALUE) {
        free(d);
        return NULL;
    }
    d->first = 1;
    return d;
}

static inline struct dirent* readdir(DIR* d) {
    if (!d) return NULL;
    if (d->first) {
        d->first = 0;
    } else {
        if (!FindNextFileA(d->hFind, &d->findData)) return NULL;
    }
    strncpy(d->ent.d_name, d->findData.cFileName, sizeof(d->ent.d_name) - 1);
    d->ent.d_name[sizeof(d->ent.d_name) - 1] = '\0';
    return &d->ent;
}

static inline int closedir(DIR* d) {
    if (!d) return -1;
    if (d->hFind != INVALID_HANDLE_VALUE) FindClose(d->hFind);
    free(d);
    return 0;
}

// -- dlopen / dlsym / dlclose → LoadLibrary/GetProcAddress/FreeLibrary -
#define RTLD_NOW    0
#define RTLD_LAZY   1
#define RTLD_LOCAL  0
#define RTLD_GLOBAL 2
#define RTLD_NOLOAD 4

static inline void* dlopen(const char* filename, int flags) {
    (void)flags;
    if (flags & RTLD_NOLOAD) {
        return (void*)GetModuleHandleA(filename);
    }
    return (void*)LoadLibraryA(filename);
}

static inline void* dlsym(void* handle, const char* symbol) {
    return (void*)GetProcAddress((HMODULE)handle, symbol);
}

static inline int dlclose(void* handle) {
    return FreeLibrary((HMODULE)handle) ? 0 : -1;
}

static inline char* dlerror(void) {
    static char buf[256];
    DWORD err = GetLastError();
    snprintf(buf, sizeof(buf), "dlopen error: %lu", err);
    return buf;
}

// =====================================================================
// pthread → Win32 replacements (for htslib thread_pool + bgzf)
// =====================================================================

// If KALLISTO_WIN32_COMPAT_H was already included (for bifrost),
// pthread types are already defined. Both use CRITICAL_SECTION now.
#ifndef KALLISTO_WIN32_COMPAT_H

// Thread pool needs RECURSIVE mutexes (thread_pool.c line 676).
// CRITICAL_SECTION is recursive; SRWLOCK is not. Use CRITICAL_SECTION.

typedef CRITICAL_SECTION pthread_mutex_t;

// On MSVC/Windows, CRITICAL_SECTION cannot be statically initialized with
// a simple {0} initializer. We use a runtime init function instead.
// PTHREAD_MUTEX_INITIALIZER is used only in hfile.c for a static mutex.
// We provide a stub that will be replaced by a runtime init.
#define PTHREAD_MUTEX_INITIALIZER {0}

// mutexattr (simplified — we only need recursive)
typedef struct { int type; } pthread_mutexattr_t;
#define PTHREAD_MUTEX_RECURSIVE 1

static inline int pthread_mutexattr_init(pthread_mutexattr_t* attr) {
    attr->type = 0;
    return 0;
}

static inline int pthread_mutexattr_settype(pthread_mutexattr_t* attr, int type) {
    attr->type = type;
    return 0;
}

static inline int pthread_mutexattr_destroy(pthread_mutexattr_t* attr) {
    (void)attr;
    return 0;
}

static inline int pthread_mutex_init(pthread_mutex_t* m, const pthread_mutexattr_t* attr) {
    (void)attr;
    InitializeCriticalSection(m);
    return 0;
}

static inline int pthread_mutex_destroy(pthread_mutex_t* m) {
    DeleteCriticalSection(m);
    return 0;
}

static inline int pthread_mutex_lock(pthread_mutex_t* m) {
    EnterCriticalSection(m);
    return 0;
}

static inline int pthread_mutex_unlock(pthread_mutex_t* m) {
    LeaveCriticalSection(m);
    return 0;
}

// -- struct timeval + gettimeofday (used by thread_pool.c) ----------------
// Skip if KALLISTO_WIN32_COMPAT_H already defines these
#ifndef KALLISTO_WIN32_COMPAT_H
#include <time.h>

static inline int gettimeofday(struct timeval* tv, void* tz) {
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
#endif

// -- condition variable -----------------------------------------------

typedef CONDITION_VARIABLE pthread_cond_t;

static inline int pthread_cond_init(pthread_cond_t* c, void* attr) {
    (void)attr;
    InitializeConditionVariable(c);
    return 0;
}

static inline int pthread_cond_destroy(pthread_cond_t* c) {
    (void)c;
    return 0;
}

static inline int pthread_cond_signal(pthread_cond_t* c) {
    WakeConditionVariable(c);
    return 0;
}

static inline int pthread_cond_broadcast(pthread_cond_t* c) {
    WakeAllConditionVariable(c);
    return 0;
}

static inline int pthread_cond_wait(pthread_cond_t* c, pthread_mutex_t* m) {
    SleepConditionVariableCS(c, m, INFINITE);
    return 0;
}

// pthread_cond_timedwait: convert struct timespec to milliseconds
static inline int pthread_cond_timedwait(pthread_cond_t* c, pthread_mutex_t* m,
                                          const struct timespec* abstime) {
    // abstime is absolute time (Unix epoch); convert to relative ms
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER now;
    now.LowPart = ft.dwLowDateTime;
    now.HighPart = ft.dwHighDateTime;
    // FILETIME = 100ns intervals since 1601
    uint64_t now_ns = (now.QuadPart - 116444736000000000ULL) * 100;
    uint64_t abstime_ns = (uint64_t)abstime->tv_sec * 1000000000ULL + abstime->tv_nsec;
    if (abstime_ns <= now_ns) {
        // Already timed out
        return 1; // ETIMEDOUT equivalent
    }
    uint64_t diff_ns = abstime_ns - now_ns;
    DWORD ms = (DWORD)(diff_ns / 1000000);
    if (ms == 0) ms = 1;
    if (!SleepConditionVariableCS(c, m, ms)) {
        DWORD err = GetLastError();
        if (err == ERROR_TIMEOUT) return 1; // ETIMEDOUT
        return -1;
    }
    return 0;
}

// -- threads ----------------------------------------------------------

typedef HANDLE pthread_t;

struct win32_hts_thread_wrapper {
    void* (*fn)(void*);
    void* arg;
};

static inline DWORD WINAPI win32_hts_thread_start(LPVOID param) {
    struct win32_hts_thread_wrapper* w = (struct win32_hts_thread_wrapper*)param;
    void* (*fn)(void*) = w->fn;
    void* arg = w->arg;
    free(w);
    fn(arg);
    return 0;
}

static inline int pthread_create(pthread_t* thread, void* attr,
                                  void* (*start_routine)(void*), void* arg) {
    (void)attr;
    struct win32_hts_thread_wrapper* w =
        (struct win32_hts_thread_wrapper*)calloc(1, sizeof(*w));
    if (!w) return -1;
    w->fn = start_routine;
    w->arg = arg;
    *thread = CreateThread(NULL, 0, win32_hts_thread_start, w, 0, NULL);
    return (*thread == NULL) ? -1 : 0;
}

static inline int pthread_join(pthread_t thread, void** retval) {
    if (retval) *retval = NULL;
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return 0;
}

static inline void pthread_exit(void* retval) {
    (void)retval;
    // In Win32, thread function just returns
}

static inline pthread_t pthread_self(void) {
    return (pthread_t)(uintptr_t)GetCurrentThreadId();
}

static inline int pthread_equal(pthread_t a, pthread_t b) {
    return a == b;
}

// pthread_kill(SIGINT) → set shutdown flag + signal condition (used in hts_tpool_kill)
// This is called in thread_pool.c to interrupt worker threads.
// We replace with a mechanism that signals all condition variables.
// thread_pool.c will need a code change instead of pthread_kill.
// Declare a stub — actual implementation requires code change in thread_pool.c.
static inline int pthread_kill(pthread_t thread, int sig) {
    (void)thread;
    (void)sig;
    // This function should not be called directly after our thread_pool.c changes.
    // If it is reached, it's a bug.
    return 0;
}

#endif // !KALLISTO_WIN32_COMPAT_H

#ifdef __cplusplus
}
#endif

#endif // _MSC_VER
#endif // HTSLIB_WIN32_COMPAT_H
