remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

include_directories(
    "${REACTOS_SOURCE_DIR}/sdk/lib/crtex"
    "${REACTOS_SOURCE_DIR}/sdk/lib/crt/wine"
    "${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine"
)

list(APPEND SOURCE
    crtex.cpp
    crtex_mingw.h
    cpuid.c
    wine/math.c
    # wine/time.c
)

if(ARCH STREQUAL "i386")
    list(APPEND SOURCE
        except/except_i386.c
    )
    list(APPEND ASM_SOURCE
        except/cpp.s
    )
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE
    )
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE
    )
endif()

set_source_files_properties(${ASM_SOURCE} PROPERTIES COMPILE_DEFINITIONS "__MINGW_IMPORT=extern;USE_MSVCRT_PREFIX;_MSVCRT_LIB_;_MSVCRT_;_MT;CRTDLL")
add_asm_files(libcrtex_mingw_asm ${ASM_SOURCE})

add_library(libcrtex_mingw OBJECT
    ${SOURCE}
    ${libcrtex_mingw_asm}
)

target_compile_definitions(libcrtex_mingw PRIVATE
    __MINGW_IMPORT=extern
    USE_MSVCRT_PREFIX
    _MSVCRT_LIB_
    _MSVCRT_
    _MT
    CRTDLL
    _MSVCR_VER=140
)

add_dependencies(libcrtex_mingw psdk errcodes asm)
