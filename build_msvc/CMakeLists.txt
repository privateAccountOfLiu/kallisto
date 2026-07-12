cmake_minimum_required(VERSION 3.16)

# Standalone build for kallisto on MSVC using pre-built libraries
# Usage: cd build_msvc && cmake . -G "Visual Studio 18 2026" -A x64

project(kallisto_msvc)

add_compile_options(/W3 /wd4996 /permissive-)

# Static CRT
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DCOMPILATION_ARCH=OFF /DENABLE_AVX2=OFF /DMAX_KMER_SIZE=32 /DNO_HTSLIB=ON")

option(USE_HDF5 "Compile with HDF5 support" ON)

# Source files
set(KALLISTO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(BIFROST_ROOT "${KALLISTO_ROOT}/ext/bifrost")
set(ZLIBNG_ROOT "${KALLISTO_ROOT}/ext/zlib-ng")
set(HDF5_ROOT "${KALLISTO_ROOT}/ext/hdf5-1.14.4-2")

file(GLOB KALLISTO_SOURCES "${KALLISTO_ROOT}/src/*.cpp")
file(GLOB KALLISTO_HEADERS "${KALLISTO_ROOT}/src/*.h" "${KALLISTO_ROOT}/src/*.hpp")

# Exclude main.cpp from core library
set(KALLISTO_CORE_SOURCES ${KALLISTO_SOURCES})
list(FILTER KALLISTO_CORE_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

# Include directories
include_directories(
    "${KALLISTO_ROOT}/src"
    "${BIFROST_ROOT}/src"
    "${ZLIBNG_ROOT}/build_mt2"
)

# Link libraries
set(BIFROST_LIB "${BIFROST_ROOT}/build_mt6/src/Release/bifrost.lib")
set(ZLIB_LIB "${ZLIBNG_ROOT}/build_mt2/Release/zlibstatic.lib")
set(HDF5_LIB "${HDF5_ROOT}/build_mt/bin/Release/libhdf5.lib")

if(USE_HDF5)
    add_compile_definitions("USE_HDF5=ON")
    include_directories("${HDF5_ROOT}/install/include")
endif()

add_executable(kallisto ${KALLISTO_SOURCES})

target_link_libraries(kallisto
    ${BIFROST_LIB}
    ${ZLIB_LIB}
)

if(USE_HDF5)
    target_link_libraries(kallisto ${HDF5_LIB} shlwapi.lib)
endif()

# Static linking flags
set_target_properties(kallisto PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:CONSOLE /STACK:8388608"
)

install(TARGETS kallisto DESTINATION bin)
