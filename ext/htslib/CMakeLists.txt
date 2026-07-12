cmake_minimum_required(VERSION 3.16)
project(htslib C)

# Static CRT
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# HTSlib source files — SAM/BAM essential subset
# CRAM, VCF/BCF excluded for initial MSVC port
set(HTSLIB_SOURCES
    kfunc.c knetfile.c kstring.c
    bgzf.c faidx.c
    hfile.c hfile_net.c hts.c md5.c
    regidx.c sam.c
    tbx.c textutils.c
    thread_pool.c
    # CRAM files needed by sam.c for reference handling
    cram/cram_io.c cram/cram_index.c
    cram/files.c cram/mFILE.c
    cram/pooled_alloc.c cram/string_alloc.c
    cram/zfio.c
)

# Include paths
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cram
    ${CMAKE_CURRENT_SOURCE_DIR}/htslib
    ${CMAKE_BINARY_DIR}
)

# Copy generated headers to build dir
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h ${CMAKE_BINARY_DIR}/config.h COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h ${CMAKE_BINARY_DIR}/version.h COPYONLY)

# Defines
add_compile_definitions(
    _USE_KNETFILE
    BGZF_MT
    _CRT_SECURE_NO_WARNINGS
    _USE_MATH_DEFINES
)

# Build static library
add_library(hts STATIC ${HTSLIB_SOURCES})

# Link with zlib (from kallisto's zlib-ng build)
set(ZLIBNG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../zlib-ng/build_mt2")
target_include_directories(hts PUBLIC ${ZLIBNG_ROOT})
target_link_libraries(hts ${ZLIBNG_ROOT}/Release/zlibstatic.lib)
