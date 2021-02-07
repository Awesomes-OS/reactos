# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.17)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
project(msvc_standard_libraries LANGUAGES CXX)

set(VCLIBS_SUFFIX "_ros" CACHE STRING "suffix for built DLL names to avoid conflicts with distributed DLLs")

if(NOT DEFINED VCLIBS_TARGET_ARCHITECTURE)
    set(VCLIBS_TARGET_ARCHITECTURE "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
endif()

remove_definitions(/Dinline=__inline -D_WIN32_WINNT=0x502 -DWINVER=0x502)

set(CMAKE_CXX_FLAGS "/X")
set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_RELEASE "")
set(CMAKE_CXX_STANDARD_LIBRARIES "kernel32.lib")
set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "kernel32.lib")
set(CMAKE_MSVC_RUNTIME_LIBRARY "")

set(STL_ROOT ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/stl)
set(UCRT_ROOT ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/ucrt)
set(VC_ROOT ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/vcruntime)
set(MINGW_CRT_ROOT ${REACTOS_SOURCE_DIR}/sdk/lib/crt)

if(ARCH STREQUAL "i386")
    set(ASM_ARCH_FLAGS "-I${UCRT_ROOT}/inc/i386 -I${VC_ROOT}/i386")
endif()

set(CMAKE_ASM_COMPILE_OBJECT
    "<CMAKE_ASM_COMPILER> /nologo /X ${ASM_ARCH_FLAGS} /Fo<OBJECT> /c /Ta <SOURCE>"
)

set_directory_properties(PROPERTIES INCLUDE_DIRECTORIES "")

if("${VCLIBS_TARGET_ARCHITECTURE}" MATCHES "^[xX]86$")
    set(VCLIBS_TARGET_ARCHITECTURE "x86")
    set(VCLIBS_I386_OR_AMD64 "i386")
    set(VCLIBS_X86_OR_X64 "x86")
    # Note that we set _WIN32_WINNT to a high level to make declarations available, but still engage downlevel
    # runtime dynamic linking by setting our own _STL_WIN32_WINNT back to Windows XP.
    add_compile_definitions(_X86_ _VCRT_WIN32_WINNT=0x0502 _STL_WIN32_WINNT=0x0502
        WINVER=0x600
        _WIN32_WINNT=0x0602 NTDDI_VERSION=NTDDI_WIN8)
    add_compile_options(/arch:IA32)
elseif(VCLIBS_TARGET_ARCHITECTURE MATCHES "^[xX]64$")
    set(VCLIBS_TARGET_ARCHITECTURE "x64")
    set(VCLIBS_I386_OR_AMD64 "amd64")
    set(VCLIBS_X86_OR_X64 "x64")
    add_compile_definitions(_AMD64_ _VCRT_WIN32_WINNT=0x0502 _STL_WIN32_WINNT=0x0502
        -DWINVER=0x600
        _WIN32_WINNT=0x0602 NTDDI_VERSION=NTDDI_WIN8)
elseif(VCLIBS_TARGET_ARCHITECTURE MATCHES "^[aA][rR][mM][vV]7$")
    set(VCLIBS_TARGET_ARCHITECTURE "arm")
    set(VCLIBS_I386_OR_AMD64 "arm")
    set(VCLIBS_X86_OR_X64 "arm")
    add_compile_definitions(_ARM_ _VCRT_WIN32_WINNT=0x0602 _STL_WIN32_WINNT=0x0602
        _WIN32_WINNT=0x0602 NTDDI_VERSION=NTDDI_WIN8)
    string(APPEND CMAKE_CXX_STANDARD_LIBRARIES " Synchronization.lib")
elseif(VCLIBS_TARGET_ARCHITECTURE MATCHES "^[aA][rR][mM]64$")
    set(VCLIBS_TARGET_ARCHITECTURE "arm64")
    set(VCLIBS_I386_OR_AMD64 "arm64")
    set(VCLIBS_X86_OR_X64 "arm64")
    add_compile_definitions(_ARM64_ _VCRT_WIN32_WINNT=0x0A00 _STL_WIN32_WINNT=0x0A00
        _WIN32_WINNT=0x0A00 NTDDI_VERSION=NTDDI_WIN10)
    string(APPEND CMAKE_CXX_STANDARD_LIBRARIES " Synchronization.lib")
else()
    message(FATAL_ERROR "Could not determine target architecture: VCLIBS_TARGET_ARCHITECTURE: ${VCLIBS_TARGET_ARCHITECTURE}")
endif()

add_compile_definitions(
    _ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH WIN32_LEAN_AND_MEAN STRICT _CRT_STDIO_ARBITRARY_WIDE_SPECIFIERS
    _CRT_DECLARE_NONSTDC_NAMES=1 )

add_compile_options(/diagnostics:caret /W3 /w14265 /w15038 /d1FastFail /guard:cf /Z7 /Gm- /Gy /Zp8 /std:c++17 /Zc:threadSafeInit- /Zl)

set(VCLIBS_DEBUG_OPTIONS "/Od")
set(VCLIBS_RELEASE_OPTIONS "/O2;/Os") # TRANSITION: Potentially remove /Os

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/out/lib/${VCLIBS_I386_OR_AMD64}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/out/lib/${VCLIBS_I386_OR_AMD64}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/out/bin/${VCLIBS_I386_OR_AMD64}")

set(CMAKE_STATIC_LINKER_FLAGS "")
set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "")
set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "")
set(CMAKE_SHARED_LINKER_FLAGS "/DEBUG:FULL /ignore:4217,4286 /RELEASE /SUBSYSTEM:Console /NODEFAULTLIB /INCREMENTAL:NO /MANIFEST:NO /DLL /profile /guard:cf /DEBUGTYPE:cv,fixup /LARGEADDRESSAWARE")
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "")

set(HEADERS
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/assert.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/complex.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/conio.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_io.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_malloc.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_math.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_math_defines.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_memcpy_s.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_memory.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_search.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_share.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_startup.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_stdio_config.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_terminate.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_wconio.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_wctype.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_wdirect.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_wio.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_wprocess.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_wstdio.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_wstdlib.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_wstring.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/corecrt_wtime.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/crtdbg.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/ctype.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/direct.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/dos.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/errno.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/fcntl.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/fenv.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/float.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/fpieee.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/inttypes.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/io.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/locale.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/malloc.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/math.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/mbctype.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/mbstring.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/memory.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/minmax.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/new.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/process.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/safeint.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/safeint_internal.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/search.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/share.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/signal.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/stdalign.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/stddef.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/stdio.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/stdlib.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/stdnoreturn.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/string.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/sys
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/sys/locking.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/sys/stat.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/sys/timeb.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/sys/types.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/sys/utime.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/tchar.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/tgmath.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/time.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/uchar.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/wchar.h
    ${REACTOS_SOURCE_DIR}/sdk/include/ucrt/wctype.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/CodeAnalysis/Warnings.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/CodeAnalysis/sourceannotations.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/Manifest/PerMonitorHighDPIAware.manifest
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/Manifest/dpiaware.manifest
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/agents.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/agile.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/ammintrin.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/amp.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/amp_graphics.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/amp_math.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/amp_short_vectors.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/amprt.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/amprt_exceptions.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/arm64_neon.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/arm64intr.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/arm_neon.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/armintr.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cfguard.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/adapter
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/algorithm
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/deque
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/functional
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/hash_map
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/hash_set
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/iterator
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/list
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/map
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/memory
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/numeric
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/queue
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/set
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/stack
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/utility
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/vector
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/xhash
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/xtree
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/cliext/xutility
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/collection.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/comdef.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/comdefsp.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/comip.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/comutil.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/concrt.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/concrtrm.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/concurrencysal.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/concurrent_priority_queue.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/concurrent_queue.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/concurrent_unordered_map.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/concurrent_unordered_set.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/concurrent_vector.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/crtdefs.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/crtversion.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/delayimp.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/dloadsup.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/dvec.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/eh.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/ehdata.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/ehdata4.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/ehdata4_export.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/ehdata_forceinclude.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/ehdata_values.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/emmintrin.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/excpt.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/fvec.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/gcroot.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/immintrin.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/internal_concurrent_hash.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/internal_split_ordered_list.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/intrin.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/intrin0.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/invkprxy.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/isa_availability.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/ivec.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/limits.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/listing.inc
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/mm3dnow.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/mmintrin.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/msclr/all.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/msclr/appdomain.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/msclr/auto_gcroot.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/msclr/auto_handle.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/msclr/com
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/msclr/com/ptr.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/msclr/event.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/msclr/gcroot.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/msclr/lock.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/msclr/marshal.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/msclr/marshal_atl.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/msclr/marshal_cppstd.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/msclr/marshal_windows.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/msclr/safebool.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/nmmintrin.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/omp.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/pgobootrun.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/pmmintrin.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/ppl.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/pplawait.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/pplcancellation_token.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/pplinterface.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/ppltasks.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/ppltaskscheduler.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/pplwin.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/rtcapi.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/rttidata.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/sal.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/setjmp.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/setjmpex.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/smmintrin.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/srv.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/stdarg.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/stdbool.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/stdint.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/tmmintrin.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/vadefs.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/varargs.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/vcclr.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/vccorlib.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/vcruntime.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/vcruntime_exception.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/vcruntime_new.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/vcruntime_new_debug.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/vcruntime_startup.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/vcruntime_string.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/vcruntime_typeinfo.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/wmmintrin.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/xmmintrin.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/xxamp.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/xxamp_inl.h
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/zmmintrin.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/__msvc_all_public_headers.hpp
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/__msvc_system_error_abi.hpp
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/algorithm
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/any
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/array
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/atomic
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/barrier
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/bit
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/bitset
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cassert
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/ccomplex
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cctype
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cerrno
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cfenv
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cfloat
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/charconv
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/chrono
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cinttypes
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/ciso646
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/climits
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/clocale
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cmath
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/codecvt
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/compare
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/complex
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/concepts
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/condition_variable
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/coroutine
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/csetjmp
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/csignal
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cstdalign
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cstdarg
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cstdbool
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cstddef
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cstdint
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cstdio
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cstdlib
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cstring
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/ctgmath
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/ctime
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cuchar
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/8859_1
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/8859_10
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/8859_13
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/8859_14
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/8859_15
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/8859_16
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/8859_2
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/8859_3
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/8859_4
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/8859_5
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/8859_6
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/8859_7
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/8859_8
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/8859_9
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/baltic
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/big5
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp037
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp1006
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp1026
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp1250
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp1251
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp1252
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp1253
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp1254
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp1255
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp1256
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp1257
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp1258
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp424
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp437
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp500
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp737
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp775
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp850
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp852
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp855
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp856
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp857
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp860
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp861
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp862
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp863
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp864
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp865
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp866
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp869
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp874
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp875
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp932
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp936
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp949
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cp950
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/cyrillic
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/ebcdic
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/euc
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/euc_0208
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/gb12345
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/gb2312
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/greek
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/iceland
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/jis
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/jis0201
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/jis_0208
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/ksc5601
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/latin2
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/one_one
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/roman
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/sjis
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/sjis_0208
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/turkish
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/utf16
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/utf8
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/utf8_utf16
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/wbuffer
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/wstring
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/xjis
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/xone_byte
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cvt/xtwo_byte
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cwchar
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/cwctype
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/deque
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/exception
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/execution
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/experimental/coroutine
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/experimental/deque
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/experimental/filesystem
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/experimental/forward_list
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/experimental/generator
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/experimental/list
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/experimental/map
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/experimental/resumable
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/experimental/set
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/experimental/string
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/experimental/unordered_map
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/experimental/unordered_set
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/experimental/vector
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/filesystem
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/forward_list
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/fstream
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/functional
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/future
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/hash_map
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/hash_set
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/initializer_list
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/iomanip
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/ios
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/iosfwd
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/iostream
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/iso646.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/istream
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/iterator
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/latch
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/limits
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/list
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/locale
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/map
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/memory
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/memory_resource
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/mutex
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/new
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/numbers
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/numeric
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/optional
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/ostream
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/queue
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/random
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/ranges
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/ratio
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/regex
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/scoped_allocator
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/semaphore
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/set
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/shared_mutex
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/span
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/sstream
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/stack
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/stdexcept
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/streambuf
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/string
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/string_view
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/strstream
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/system_error
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/thread
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/tuple
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/type_traits
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/typeindex
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/typeinfo
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/unordered_map
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/unordered_set
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/use_ansi.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/utility
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/valarray
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/variant
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/vector
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/version
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xatomic.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xatomic_wait.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xbit_ops.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xcall_once.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xcharconv.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xcharconv_ryu.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xcharconv_ryu_tables.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xerrc.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xfacet
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xfilesystem_abi.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xhash
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xiosbase
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xkeycheck.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xlocale
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xlocbuf
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xlocinfo
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xlocinfo.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xlocmes
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xlocmon
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xlocnum
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xloctime
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xmemory
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xnode_handle.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xpolymorphic_allocator.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xsmf_control.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xstddef
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xstring
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xthreads.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xtimec.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xtr1common
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xtree
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/xutility
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/ymath.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/yvals.h
    ${REACTOS_SOURCE_DIR}/sdk/include/c++/stl/yvals_core.h
)

set(UCRT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/env/getenv.cpp
    ${CMAKE_CURRENT_LIST_DIR}/heap/align.cpp
    ${UCRT_ROOT}/heap/calloc.cpp
    ${UCRT_ROOT}/heap/calloc_base.cpp
    ${CMAKE_CURRENT_LIST_DIR}/heap/debug_heap.cpp
    ${UCRT_ROOT}/heap/expand.cpp
    ${UCRT_ROOT}/heap/free.cpp
    ${UCRT_ROOT}/heap/free_base.cpp
    ${UCRT_ROOT}/heap/heap_handle.cpp
    ${UCRT_ROOT}/heap/heapchk.cpp
    ${UCRT_ROOT}/heap/heapmin.cpp
    ${UCRT_ROOT}/heap/heapwalk.cpp
    ${UCRT_ROOT}/heap/malloc.cpp
    ${UCRT_ROOT}/heap/malloc_base.cpp
    ${UCRT_ROOT}/heap/msize.cpp
    ${UCRT_ROOT}/heap/new_handler.cpp
    ${UCRT_ROOT}/heap/new_mode.cpp
    ${UCRT_ROOT}/heap/realloc.cpp
    ${UCRT_ROOT}/heap/realloc_base.cpp
    ${UCRT_ROOT}/heap/recalloc.cpp
    ${UCRT_ROOT}/internal/locks.cpp
    ${UCRT_ROOT}/internal/OutputDebugStringA.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/initialization.cpp
    ${UCRT_ROOT}/internal/report_runtime_error.cpp
    ${UCRT_ROOT}/internal/shared_initialization.cpp
    ${UCRT_ROOT}/locale/initctype.cpp
    ${UCRT_ROOT}/locale/initmon.cpp
    ${UCRT_ROOT}/locale/initnum.cpp
    ${UCRT_ROOT}/locale/inittime.cpp
    ${CMAKE_CURRENT_LIST_DIR}/locale/ctype.cpp
    ${UCRT_ROOT}/locale/localeconv.cpp
    ${UCRT_ROOT}/locale/locale_refcounting.cpp
    ${UCRT_ROOT}/locale/locale_update.cpp
    ${UCRT_ROOT}/locale/glstatus.cpp
    ${UCRT_ROOT}/locale/wsetlocale.cpp
    ${UCRT_ROOT}/initializers/locale_initializer.cpp
    ${UCRT_ROOT}/initializers/multibyte_initializer.cpp
    ${UCRT_ROOT}/initializers/timeset_initializer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/locale/nlsdata.cpp
    ${UCRT_ROOT}/misc/errno.cpp
    ${UCRT_ROOT}/misc/exception_filter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/misc/signal.cpp
    ${UCRT_ROOT}/string/strncnt.cpp
    ${UCRT_ROOT}/string/wcsdup.cpp
    ${UCRT_ROOT}/string/wcsnicmp.cpp
    ${UCRT_ROOT}/time/days.cpp
    ${CMAKE_CURRENT_LIST_DIR}/time/strftime.cpp
    ${CMAKE_CURRENT_LIST_DIR}/time/wcsftime.cpp
    ${UCRT_ROOT}/time/timeset.cpp
    ${UCRT_ROOT}/time/tzset.cpp
    ${CMAKE_CURRENT_LIST_DIR}/invalid_parameter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/per_thread_data.cpp
    ${CMAKE_CURRENT_LIST_DIR}/terminate.cpp
    ${CMAKE_CURRENT_LIST_DIR}/winapi_thunks.cpp
    ${CMAKE_CURRENT_LIST_DIR}/startup/abort.cpp
    ${CMAKE_CURRENT_LIST_DIR}/startup/assert.cpp
    ${CMAKE_CURRENT_LIST_DIR}/startup/exit.cpp
    ${CMAKE_CURRENT_LIST_DIR}/startup/initterm.cpp
    ${CMAKE_CURRENT_LIST_DIR}/startup/onexit.cpp
    ${CMAKE_CURRENT_LIST_DIR}/matherr.cpp
)

set(VCRT_SOURCES
    ${VC_ROOT}/vcruntime/delete_array.cpp
    ${VC_ROOT}/vcruntime/delete_array_align.cpp
    ${VC_ROOT}/vcruntime/delete_array_align_nothrow.cpp
    ${VC_ROOT}/vcruntime/delete_array_nothrow.cpp
    ${VC_ROOT}/vcruntime/delete_array_size.cpp
    ${VC_ROOT}/vcruntime/delete_array_size_align.cpp
    ${VC_ROOT}/vcruntime/delete_debug.cpp
    ${VC_ROOT}/vcruntime/delete_scalar.cpp
    ${VC_ROOT}/vcruntime/delete_scalar_align.cpp
    ${VC_ROOT}/vcruntime/delete_scalar_align_nothrow.cpp
    ${VC_ROOT}/vcruntime/delete_scalar_nothrow.cpp
    ${VC_ROOT}/vcruntime/delete_scalar_size.cpp
    ${VC_ROOT}/vcruntime/delete_scalar_size_align.cpp
    ${VC_ROOT}/vcruntime/ehstate.cpp
    ${VC_ROOT}/vcruntime/ehhelpers.cpp
    ${VC_ROOT}/vcruntime/ehhelpers.h
    ${VC_ROOT}/vcruntime/jbcxrval.c
    ${CMAKE_CURRENT_LIST_DIR}/gs_cookie.c
    ${CMAKE_CURRENT_LIST_DIR}/gs_report.c
    ${CMAKE_CURRENT_LIST_DIR}/gs_support.c
    ${CMAKE_CURRENT_LIST_DIR}/guard_support.c
    ${VC_ROOT}/vcruntime/locks.cpp
    ${VC_ROOT}/vcruntime/frame.cpp
    ${VC_ROOT}/vcruntime/new_array.cpp
    ${VC_ROOT}/vcruntime/new_array_align.cpp
    ${VC_ROOT}/vcruntime/new_array_align_nothrow.cpp
    ${VC_ROOT}/vcruntime/new_array_nothrow.cpp
    ${VC_ROOT}/vcruntime/new_debug.cpp
    ${VC_ROOT}/vcruntime/new_scalar.cpp
    ${VC_ROOT}/vcruntime/new_scalar_align.cpp
    ${VC_ROOT}/vcruntime/new_scalar_align_nothrow.cpp
    ${VC_ROOT}/vcruntime/new_scalar_nothrow.cpp
    ${VC_ROOT}/vcruntime/per_thread_data.cpp
    ${VC_ROOT}/vcruntime/pesect.c
    ${VC_ROOT}/vcruntime/purevirt.cpp
    ${VC_ROOT}/vcruntime/purevirt_data.cpp
    ${VC_ROOT}/vcruntime/std_exception.cpp
    ${VC_ROOT}/vcruntime/unexpected.cpp
    ${VC_ROOT}/vcruntime/user.cpp
    ${VC_ROOT}/vcruntime/ucrt_detection.c
    ${VC_ROOT}/vcruntime/ucrt_stubs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/vcruntime/utility.cpp
    ${CMAKE_CURRENT_LIST_DIR}/throw.cpp
    ${CMAKE_CURRENT_LIST_DIR}/utility_desktop.cpp
    ${CMAKE_CURRENT_LIST_DIR}/vcruntime/initialization.cpp
    ${CMAKE_CURRENT_LIST_DIR}/vcruntime/initializers.cpp
    ${CMAKE_CURRENT_LIST_DIR}/vcruntime/std_type_info.cpp
    ${VC_ROOT}/vcruntime/std_type_info_static.cpp
    ${VC_ROOT}/linkopts/legacy_stdio_definitions.cpp

    ${CMAKE_CURRENT_LIST_DIR}/mscpprt.cpp
)

set(UCRT_DEBUG_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/dbgrpt.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dbgrptt.cpp
)

set(UCRT_MBCS_SOURCES
    # ${UCRT_ROOT}/convert/ismbstr.cpp
    # ${UCRT_ROOT}/mbstring/ismbalnm.cpp
    # ${UCRT_ROOT}/mbstring/ismbalph.cpp
    ${CMAKE_CURRENT_LIST_DIR}/mbstring/ismbbyte.cpp
    # ${UCRT_ROOT}/mbstring/ismbdgt.cpp
    # ${UCRT_ROOT}/mbstring/ismbgrph.cpp
    # ${UCRT_ROOT}/mbstring/ismbknj.cpp
    # ${UCRT_ROOT}/mbstring/ismblgl.cpp
    # ${UCRT_ROOT}/mbstring/ismblwr.cpp
    # ${UCRT_ROOT}/mbstring/ismbprn.cpp
    # ${UCRT_ROOT}/mbstring/ismbpunc.cpp
    # ${UCRT_ROOT}/mbstring/ismbsle.cpp
    # ${UCRT_ROOT}/mbstring/ismbspc.cpp
    # ${UCRT_ROOT}/mbstring/ismbupr.cpp
    # ${UCRT_ROOT}/mbstring/mbbtype.cpp
    # ${UCRT_ROOT}/mbstring/mbccpy.cpp
    # ${UCRT_ROOT}/mbstring/mbccpy_s.cpp
    # ${UCRT_ROOT}/mbstring/mbccpy_s_l.cpp
    # ${UCRT_ROOT}/mbstring/mbclen.cpp
    # ${UCRT_ROOT}/mbstring/mbclevel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/mbstring/mbctype.cpp
    # ${UCRT_ROOT}/mbstring/mbsbtype.cpp
    # ${UCRT_ROOT}/mbstring/mbscat_s.cpp
    # ${UCRT_ROOT}/mbstring/mbscat_s_l.cpp
    # ${UCRT_ROOT}/mbstring/mbschr.cpp
    # ${UCRT_ROOT}/mbstring/mbscmp.cpp
    # ${UCRT_ROOT}/mbstring/mbscoll.cpp
    # ${UCRT_ROOT}/mbstring/mbscpy_s.cpp
    # ${UCRT_ROOT}/mbstring/mbscpy_s_l.cpp
    # ${UCRT_ROOT}/mbstring/mbscspn.cpp
    # ${UCRT_ROOT}/mbstring/mbsdec.cpp
    # ${UCRT_ROOT}/mbstring/mbsicmp.cpp
    # ${UCRT_ROOT}/mbstring/mbsicoll.cpp
    # ${UCRT_ROOT}/mbstring/mbsinc.cpp
    # ${UCRT_ROOT}/mbstring/mbslen.cpp
    # ${UCRT_ROOT}/mbstring/mbslen_s.cpp
    # ${UCRT_ROOT}/mbstring/mbslwr.cpp
    # ${UCRT_ROOT}/mbstring/mbsnbcat.cpp
    # ${UCRT_ROOT}/mbstring/mbsnbcat_s.cpp
    # ${UCRT_ROOT}/mbstring/mbsnbcat_s_l.cpp
    # ${UCRT_ROOT}/mbstring/mbsnbcmp.cpp
    # ${UCRT_ROOT}/mbstring/mbsnbcnt.cpp
    # ${UCRT_ROOT}/mbstring/mbsnbcol.cpp
    # ${UCRT_ROOT}/mbstring/mbsnbcpy.cpp
    # ${UCRT_ROOT}/mbstring/mbsnbcpy_s.cpp
    # ${UCRT_ROOT}/mbstring/mbsnbcpy_s_l.cpp
    # ${UCRT_ROOT}/mbstring/mbsnbicm.cpp
    # ${UCRT_ROOT}/mbstring/mbsnbico.cpp
    # ${UCRT_ROOT}/mbstring/mbsnbset.cpp
    # ${UCRT_ROOT}/mbstring/mbsnbset_s.cpp
    # ${UCRT_ROOT}/mbstring/mbsnbset_s_l.cpp
    # ${UCRT_ROOT}/mbstring/mbsncat.cpp
    # ${UCRT_ROOT}/mbstring/mbsncat_s.cpp
    # ${UCRT_ROOT}/mbstring/mbsncat_s.inl
    # ${UCRT_ROOT}/mbstring/mbsncat_s_l.cpp
    # ${UCRT_ROOT}/mbstring/mbsnccnt.cpp
    # ${UCRT_ROOT}/mbstring/mbsncmp.cpp
    # ${UCRT_ROOT}/mbstring/mbsncoll.cpp
    # ${UCRT_ROOT}/mbstring/mbsncpy.cpp
    # ${UCRT_ROOT}/mbstring/mbsncpy_s.cpp
    # ${UCRT_ROOT}/mbstring/mbsncpy_s.inl
    # ${UCRT_ROOT}/mbstring/mbsncpy_s_l.cpp
    # ${UCRT_ROOT}/mbstring/mbsnextc.cpp
    # ${UCRT_ROOT}/mbstring/mbsnicmp.cpp
    # ${UCRT_ROOT}/mbstring/mbsnicol.cpp
    # ${UCRT_ROOT}/mbstring/mbsninc.cpp
    # ${UCRT_ROOT}/mbstring/mbsnset.cpp
    # ${UCRT_ROOT}/mbstring/mbsnset_s.cpp
    # ${UCRT_ROOT}/mbstring/mbsnset_s.inl
    # ${UCRT_ROOT}/mbstring/mbsnset_s_l.cpp
    # ${UCRT_ROOT}/mbstring/mbspbrk.cpp
    # ${UCRT_ROOT}/mbstring/mbsrchr.cpp
    # ${UCRT_ROOT}/mbstring/mbsrev.cpp
    # ${UCRT_ROOT}/mbstring/mbsset.cpp
    # ${UCRT_ROOT}/mbstring/mbsset_s.cpp
    # ${UCRT_ROOT}/mbstring/mbsset_s_l.cpp
    # ${UCRT_ROOT}/mbstring/mbsspn.cpp
    # ${UCRT_ROOT}/mbstring/mbsspnp.cpp
    # ${UCRT_ROOT}/mbstring/mbsstr.cpp
    # ${UCRT_ROOT}/mbstring/mbstok.cpp
    # ${UCRT_ROOT}/mbstring/mbstok_s.cpp
    # ${UCRT_ROOT}/mbstring/mbsupr.cpp
    # ${UCRT_ROOT}/mbstring/mbtohira.cpp
    # ${UCRT_ROOT}/mbstring/mbtokata.cpp
    # ${UCRT_ROOT}/mbstring/mbtolwr.cpp
    # ${UCRT_ROOT}/mbstring/mbtoupr.cpp
    # ${UCRT_ROOT}/mbstring/tojisjms.cpp
    # ${UCRT_ROOT}/mbstring/tombbmbc.cpp
)

if(ARCH STREQUAL "i386")
    set(UCRT_ASM_SOURCES
    )
    set(VC_ASM_SOURCES
        ${VC_ROOT}/i386/ehprolog.asm
        ${VC_ROOT}/i386/except.inc
        ${CMAKE_CURRENT_LIST_DIR}/i386/exsup.asm
        ${VC_ROOT}/i386/exsup.inc
        ${VC_ROOT}/i386/lowhelpr.asm
        ${CMAKE_CURRENT_LIST_DIR}/i386/notify.asm
    )
    set(VC_CXX_SOURCES
        ${VC_ROOT}/vcruntime/x86_exception_filter.cpp
        ${VC_ROOT}/i386/chandler4.c
        ${VC_ROOT}/i386/ehprolg2.c
        ${VC_ROOT}/i386/ehprolg3.c
        ${VC_ROOT}/i386/ehprolg3a.c
        ${VC_ROOT}/i386/trnsctrl.cpp
    )
elseif(ARCH STREQUAL "amd64")
    set(UCRT_ASM_SOURCES
    )
    set(VC_ASM_SOURCES
        ${VC_ROOT}/x64/handlers.asm
    )
    set(UCRT_CXX_SOURCES
    )
    set(VC_CXX_SOURCES
        ${VC_ROOT}/x64/gshandlereh.cpp
        ${VC_ROOT}/x64/gshandlereh4.cpp
        ${VC_ROOT}/vcruntime/riscchandler.cpp
        ${VC_ROOT}/vcruntime/risctrnsctrl.cpp
    )
elseif(ARCH STREQUAL "arm")
    set(VC_ASM_SOURCES
    )
    set(UCRT_CXX_SOURCES
    )
    set(VC_CXX_SOURCES
        ${VC_ROOT}/arm/gshandlereh.cpp
        ${VC_ROOT}/arm/helpexcept.cpp
    )
elseif(ARCH STREQUAL "arm64")
    set(UCRT_ASM_SOURCES
    )
    set(VC_ASM_SOURCES
        ${VC_ROOT}/arm64/handlers.asm
    )
    set(VC_CXX_SOURCES
        ${VC_ROOT}/arm64/gshandlereh.cpp
    )
endif()

list(APPEND UCRT_SOURCES ${UCRT_ASM_SOURCES})
list(APPEND UCRT_SOURCES ${UCRT_CXX_SOURCES})
list(APPEND VCRT_SOURCES ${VC_ASM_SOURCES})
list(APPEND VCRT_SOURCES ${VC_CXX_SOURCES})

list(APPEND SOURCES
    ${REACTOS_SOURCE_DIR}/sdk/include/vcruntime/delayhlp.cpp
)

add_compile_definitions(
    _CRTBLD _VCRT_ALLOW_INTERNALS _HAS_OLD_IOSTREAMS_MEMBERS=1

    _CRT_DECLARE_GLOBAL_VARIABLES_DIRECTLY
    _CRT_GLOBAL_STATE_ISOLATION
    _UCRT_ENCLAVE_BUILD
    _NO_CRT_STDIO_INLINE
    DISABLE_NLG
)

set(STL_CONFIG_DEFINITIONS
    CRTDLL2
)

include_directories(
    "${REACTOS_SOURCE_DIR}/sdk/lib/crtex"
    "${REACTOS_SOURCE_DIR}/sdk/include/c++/stl"
    "${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/vcruntime/concrt"
    "${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/vcruntime/vcruntime"
    "${REACTOS_SOURCE_DIR}/sdk/include/ucrt"
    "${REACTOS_SOURCE_DIR}/sdk/include/vcruntime"
    "${UCRT_ROOT}/inc"
    "${REACTOS_SOURCE_DIR}/sdk/include"
    "${REACTOS_SOURCE_DIR}/sdk/include/ddk"
    "${REACTOS_SOURCE_DIR}/sdk/include/ndk"
    "${REACTOS_SOURCE_DIR}/sdk/include/psdk-ms"
    "${REACTOS_SOURCE_DIR}/sdk/include/reactos"
    "${REACTOS_BINARY_DIR}/sdk/include"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(FLAVOR_CONFIG_DEFINITIONS "_DEBUG")
    set(FLAVOR_CONFIG_COMPILE_OPTIONS "${VCLIBS_DEBUG_OPTIONS}")
    set(GL_FLAG "")
    set(FLAVOR_CONFIG_LINK_OPTIONS "/opt:ref,noicf")
    list(APPEND UCRT_SOURCES ${UCRT_DEBUG_SOURCES})
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(FLAVOR_CONFIG_DEFINITIONS "")
    set(FLAVOR_CONFIG_COMPILE_OPTIONS "${VCLIBS_RELEASE_OPTIONS}")
    set(GL_FLAG "/GL")
    set(FLAVOR_CONFIG_LINK_OPTIONS "/LTCG;/opt:ref,icf")
endif()

# STATIC
add_library(libcrtex_ucrt OBJECT ${UCRT_SOURCES})
target_compile_definitions(libcrtex_ucrt PRIVATE "${FLAVOR_CONFIG_DEFINITIONS};_CORECRT_BUILD")
target_compile_options(libcrtex_ucrt PRIVATE "${FLAVOR_CONFIG_COMPILE_OPTIONS};/EHsc")

add_library(libcrtex_ucrt_mbcs OBJECT ${UCRT_MBCS_SOURCES})
target_compile_definitions(libcrtex_ucrt_mbcs PRIVATE "${FLAVOR_CONFIG_DEFINITIONS};_CORECRT_BUILD;_MBCS")
target_compile_options(libcrtex_ucrt_mbcs PRIVATE "${FLAVOR_CONFIG_COMPILE_OPTIONS};/EHsc")

add_library(libcrtex_vcrt OBJECT ${VCRT_SOURCES})
target_compile_definitions(libcrtex_vcrt PRIVATE "${FLAVOR_CONFIG_DEFINITIONS};_VCRT_BUILD")
target_compile_options(libcrtex_vcrt PRIVATE "${FLAVOR_CONFIG_COMPILE_OPTIONS};/EHsc")

add_library(libcrtex_stl OBJECT ${IMPLIB_SOURCES} ${SOURCES})
target_compile_definitions(libcrtex_stl PRIVATE "${FLAVOR_CONFIG_DEFINITIONS};${STL_CONFIG_DEFINITIONS}")
target_compile_options(libcrtex_stl PRIVATE "${FLAVOR_CONFIG_COMPILE_OPTIONS};/EHsc")

add_library(libcrtex_ms STATIC ${HEADERS})
target_compile_definitions(libcrtex_ms PRIVATE "${FLAVOR_CONFIG_DEFINITIONS}")
target_compile_options(libcrtex_ms PRIVATE "${FLAVOR_CONFIG_COMPILE_OPTIONS};/EHsc")
target_link_libraries(libcrtex_ms PRIVATE libcrtex_ucrt libcrtex_ucrt_mbcs libcrtex_vcrt libcrtex_stl)
