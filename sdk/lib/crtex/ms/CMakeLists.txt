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
    ${CMAKE_CURRENT_LIST_DIR}/delayhlp.cpp
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
    "${VC_ROOT}/concrt"
    "${VC_ROOT}/vcruntime"
    "${UCRT_ROOT}/inc"
    "${REACTOS_SOURCE_DIR}/sdk/include"
    "${REACTOS_SOURCE_DIR}/sdk/include/ndk"
    "${REACTOS_SOURCE_DIR}/sdk/include/reactos"
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

add_library(libcrtex_ms STATIC)
target_compile_definitions(libcrtex_ms PRIVATE "${FLAVOR_CONFIG_DEFINITIONS}")
target_compile_options(libcrtex_ms PRIVATE "${FLAVOR_CONFIG_COMPILE_OPTIONS};/EHsc")
target_link_libraries(libcrtex_ms PRIVATE libcrtex_ucrt libcrtex_ucrt_mbcs libcrtex_vcrt libcrtex_stl)
