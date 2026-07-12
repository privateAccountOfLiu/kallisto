# kallisto

__kallisto__ is a program for quantifying abundances of transcripts from
RNA-Seq data, or more generally of target sequences using high-throughput
sequencing reads. It is based on the novel idea of _pseudoalignment_ for
rapidly determining the compatibility of reads with targets, without the need
for alignment. On benchmarks with standard RNA-Seq data, __kallisto__ can
quantify 30 million human bulk RNA-seq reads in less than 3  minutes on a Mac desktop
computer using only the read sequences and a transcriptome index that
itself takes than 10 minutes to build. Pseudoalignment of reads
preserves the key information needed for quantification, and __kallisto__
is therefore not only fast, but also comparably accurate to other existing
quantification tools. In fact, because the pseudoalignment procedure is
robust to errors in the reads, in many benchmarks __kallisto__
significantly outperforms existing tools. The __kallisto__ algorithms are described in more detail in:

NL Bray, H Pimentel, P Melsted and L Pachter, [Near optimal probabilistic RNA-seq quantification](http://www.nature.com/nbt/journal/v34/n5/abs/nbt.3519.html), Nature Biotechnology __34__, p 525--527 (2016).

Scripts reproducing all the results of the paper are available [here](https://github.com/pachterlab/kallisto_paper_analysis).

__kallisto__ quantified bulk RNA-Seq can be analyzed with [__sleuth__](https://github.com/pachterlab/sleuth/).

__kallisto__ can be used together with [__bustools__](https://bustools.github.io/) to pre-process single-cell RNA-seq data. See the [kallistobus.tools](https://www.kallistobus.tools/) website for instructions.

## Manual

Please visit http://pachterlab.github.io/kallisto/manual.html for the manual.

## License

__kallisto__ is distributed under the BSD-2 license. The license is distributed with __kallisto__ in the file `license.txt`, which is also viewable [here](https://pachterlab.github.io/kallisto/download). Please read the license before using __kallisto__.

## Getting help

For help running __kallisto__, please post to the [kallisto-and-applications Google Group](https://groups.google.com/forum/#!forum/kallisto-and-applications).

## Reporting bugs

Please report bugs to the [Github issues page](https://github.com/pachterlab/kallisto/issues)

## Windows MSVC Port (x86_64)

kallisto 现已支持 **Windows x86_64 MSVC** 原生编译与静态链接。

### 适配概述

原始的 kallisto 仅为 macOS 和 Linux 设计，使用了大量 POSIX/Pthread API。本次适配将其所有 Unix 特定调用替换为 Windows 原生 API，实现了 MSVC 编译器的完整支持。

### POSIX → Windows API 替换清单

| 类别 | POSIX / GCC 原始 API | Windows MSVC 替代 |
|------|----------------------|-------------------|
| 文件信息 | `stat()`, `struct stat`, `S_ISDIR()` | `_stat64`, `struct _stat64`, `(_S_IFDIR)` |
| 目录操作 | `mkdir(path, mode_t)` | `_mkdir(path)` |
| 文件描述符 | `fileno(stdin)` | `_fileno(stdin)` |
| CLI 参数 | `getopt_long()`, `struct option` | 自实现 `src/getopt_port.h` |
| 时间 | `gettimeofday()`, `struct timeval` | `GetSystemTimeAsFileTime()` |
| 128位乘法 | `__uint128_t` | `_umul128()` |
| 位扫描 | `__builtin_ffsll()` | `_BitScanForward64()` |
| 原子加法 | `__sync_fetch_and_add()` | `InterlockedExchangeAdd[64]()` |
| 原子或 | `__sync_fetch_and_or()` | `InterlockedOr64()` |
| 原子与 | `__sync_fetch_and_and()` | `InterlockedAnd64()` |
| **互斥锁** | `pthread_mutex_t` | **`SRWLOCK`** |
| **互斥锁操作** | `pthread_mutex_init/lock/unlock/destroy` | **`InitializeSRWLock/AcquireSRWLockExclusive/ReleaseSRWLockExclusive`** |
| **线程创建** | `pthread_create()` | **`CreateThread()`**（含 wrapper） |
| **线程等待** | `pthread_join()` | **`WaitForSingleObject()`** |

### 修改的源文件

**新建文件（兼容层）：**

| 文件 | 说明 |
|------|------|
| `src/win32_compat.h` | 全部 POSIX→Win32 和 Pthread→Win32 替换宏/内联函数 |
| `src/getopt_port.h` | GNU `getopt_long` 纯头文件实现 |
| `ext/bifrost/src/win32_compat_bifrost.h` | Bifrost 库的兼容桥接头 |
| `build_msvc/CMakeLists.txt` | MSVC 独立构建配置（预编译库模式） |

**修改的源文件：**

| 文件 | 修改内容 |
|------|---------|
| `src/common.h` | 引入 `win32_compat.h` |
| `src/main.cpp` | `getopt_long` 替换, `my_mkdir` 简化, `const_cast` |
| `src/KmerIndex.cpp` | `my_mkdir_kmer_index` 简化 |
| `ext/bifrost/src/BooPHF.h` | pthread→Win32 线程替换, `sys/time.h` 替换 |
| `ext/bifrost/src/Common.hpp` | 引入 `win32_compat_bifrost.h` |
| `ext/bifrost/src/CompactedDBG.hpp` | `getopt.h` MSVC guard |
| `ext/bifrost/src/KmerStream.hpp` | `getopt.h` MSVC guard |
| `ext/bifrost/src/roaring.h` | `_POSIX_C_SOURCE` MSVC guard |
| `ext/bifrost/src/roaring.c` | `__uint128_t→_umul128` MSVC 替换 |
| `ext/bifrost/src/DataStorage.tcc` | 上游 bug 修复: `o.sz_link→o.unitig_cs_link` |
| `ext/bifrost/CMakeLists.txt` | GCC flag guard |
| `ext/bifrost/src/CMakeLists.txt` | `Bifrost.cpp` 排除，pthread 条件链接 |
| `CMakeLists.txt` (root) | MSVC 构建路径 |
| `src/CMakeLists.txt` | pthread/librt 条件链接 |

### 构建方式

```powershell
# 1. 构建 zlib-ng (静态, /MT)
cmake -S ext/zlib-ng -B ext/zlib-ng/build_mt -G "Visual Studio 17 2022" -A x64 `
  -DZLIB_COMPAT=ON -DZLIB_ENABLE_TESTS=OFF `
  "-DCMAKE_C_FLAGS=//MT" "-DCMAKE_C_FLAGS_RELEASE=//MT //O2 //DNDEBUG"
cmake --build ext/zlib-ng/build_mt --config Release

# 2. 构建 Bifrost (静态, /MT)
cmake -S ext/bifrost -B ext/bifrost/build_mt -G "Visual Studio 17 2022" -A x64 `
  -DMAX_KMER_SIZE=32 -DCOMPILATION_ARCH=OFF -DENABLE_AVX2=OFF `
  "-DZLIB_LIBRARY=ext/zlib-ng/build_mt/Release/zlibstatic.lib" `
  "-DZLIB_INCLUDE_DIR=ext/zlib-ng/build_mt" `
  "-DCMAKE_C_FLAGS=//MT" "-DCMAKE_CXX_FLAGS=//MT" `
  "-DCMAKE_C_FLAGS_RELEASE=//MT //O2 //DNDEBUG" `
  "-DCMAKE_CXX_FLAGS_RELEASE=//MT //O2 //DNDEBUG"
cmake --build ext/bifrost/build_mt --config Release

# 3. 构建 kallisto
cmake -S build_msvc -B build_msvc -G "Visual Studio 17 2022" -A x64
cmake --build build_msvc --config Release

# 产物: build_msvc/Release/kallisto.exe
```

### 静态链接验证

```powershell
dumpbin /dependents build_msvc/Release/kallisto.exe
# 仅依赖: KERNEL32.dll (系统内核，无法避免)
# 无 MSVCP*.dll, VCRUNTIME*.dll 等 CRT 运行时依赖
```

### 兼容性说明

- **HDF5 支持**: 需要预先编译 Windows 静态 HDF5 库
- **BAM (HTSlib) 支持**: HTSlib 的 thread_pool 使用大量 pthread，需要单独的替换工作
- 当前版本不支持 BAM 输入/输出和 HDF5 输出

## Development and pull requests

We typically develop on separate branches, then merge into devel once features
have been sufficiently tested. `devel` is the latest, stable, development
branch. `master` is used only for official releases and is considered to be
stable. If you submit a pull request (thanks!) please make sure to request to
merge into `devel` and NOT `master`. Merges usually only go into `master`, but
not out.
