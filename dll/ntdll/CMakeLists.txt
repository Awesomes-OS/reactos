# Embed RTC libs
if (STACK_PROTECTOR)
    target_sources(libntdll PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
endif()

add_definitions(
    -D__NTDLL__
    -D_NTOSKRNL_
    -D_NTSYSTEM_
    -D_NTDLLBUILD_
    -DCRTDLL
)

if(DBG)
    add_definitions(-D_DEBUG)
endif()

add_subdirectory(ldr)

spec2def(ntdll.dll def/ntdll.spec ADD_IMPORTLIB)
aliasobj(${CMAKE_CURRENT_LIST_DIR}/def/ntdll-uvcrt.txt)

include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)

list(APPEND SOURCE
    csr/api.c
    csr/capture.c
    csr/connect.c
    dbg/dbgui.c
    etw/eventlog.c
    etw/trace.c
    rtl/libsupp.c
    rtl/process.c
    rtl/thread.c
    rtl/uilist.c
    rtl/version.c
    threadpool/threadpool.c
)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE dispatch/i386/dispatch.S)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE dispatch/amd64/dispatch.S)
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE dispatch/arm/stubs_asm.s)
else()
    list(APPEND SOURCE dispatch/dispatch.c)
endif()

add_asm_files(ntdll_asm ${ASM_SOURCE})

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/ntdll_stubs.c)

add_library(ntdll MODULE
    ${SOURCE}
    ${ntdll_asm}
    ${PCH_SKIP_SOURCE}
    def/ntdll.rc
    ${CMAKE_CURRENT_BINARY_DIR}/aliasobj_output.obj
    ${CMAKE_CURRENT_BINARY_DIR}/ntdll.def)

set_module_type(ntdll win32dll ENTRYPOINT 0)
#############################################
## HACK FOR MSVC COMPILATION WITH win32dll ##
set_subsystem(ntdll console)
################# END  HACK #################

if(MSVC)
    add_target_link_flags(ntdll "/RELEASE")
endif()

if (STACK_PROTECTOR)
    target_sources(ntdll PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
endif()

# target_link_libraries(ntdll ntdll_ldr rtl ntdllsys libcntpr uuid ${PSEH_LIB})
# target_link_libraries(ntdll ntdll_ldr stl-static-ehsc rtl ntdllsys libuvcrt::libuvcrt uuid legacy_stdio_definitions.lib legacy_stdio_wide_specifiers.lib)
# target_link_libraries(ntdll ntdll_ldr libcxx_static libcrtex rtl ntdllsys libcntpr uuid ${PSEH_LIB})
target_link_libraries(ntdll ntdll_ldr stl-static-ehsc libcrtex rtl ntdllsys libcntpr uuid ${PSEH_LIB})
add_pch(ntdll include/ntdll.h "${PCH_SKIP_SOURCE}")
add_dependencies(ntdll ntstatus asm)
add_cd_file(TARGET ntdll DESTINATION reactos/system32 NO_CAB FOR all)
