/* Minimal config.h for HTSlib on MSVC (Windows x86_64)
 * Generated for HTSlib 1.4.1 bundled with kallisto */

#ifndef HTSLIB_CONFIG_H
#define HTSLIB_CONFIG_H

/* Platform */
#define HTSLIB_VERSION "1.4.1"

/* MSVC doesn't have drand48; use rand() fallback */
/* #undef HAVE_DRAND48 */

/* We have _mkdir, _stat64, _fileno, etc. */
#define HAVE_CLOSESOCKET 1
#define HAVE_SETMODE 1

/* No mmap — use fread fallback in mFILE.c */
/* #undef HAVE_MMAP */

/* No clock_gettime — use GetSystemTimeAsFileTime */
/* #undef HAVE_CLOCK_GETTIME */

/* Zlib is available */
#define HAVE_LIBZ 1

/* No bz2/lzma by default (can be added later) */
/* #undef HAVE_LIBBZ2 */
/* #undef HAVE_LIBLZMA */

/* C99 features available on MSVC 2015+ */
#define HAVE__STRTOI64 1
#define HAVE_STDINT_H 1

/* Function attributes — not available on MSVC */
#define HTS_FORMAT(K, M, A)
#define HTS_NORETURN __declspec(noreturn)

/* Plugin extension */
#define PLUGIN_EXT ".dll"

/* rANS_STATIC codec */
#define RANS_STATIC 1

/* Use 64-bit file offsets */
#define HAVE_FSEEKO 1

/* off_t — defined in htslib_win32_compat.h via sys/types.h */

/* Avoid conflicting with MSVC's ERROR in <windows.h> */
#ifdef ERROR
#undef ERROR
#endif

#endif /* HTSLIB_CONFIG_H */
