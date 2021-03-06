add_subdirectory(kernel32_vista)

add_definitions(-D_KERNEL32_)

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)
spec2def(kernel32.dll kernel32.spec ADD_IMPORTLIB)

aliasobj(${CMAKE_CURRENT_LIST_DIR}/kernel32-alias.txt)

# Shared between kernel32 and kernel32_vista
add_library(kernel32_shared
    client/shared_utils.c
    client/file/fileutils.c
    ${CMAKE_CURRENT_BINARY_DIR}/aliasobj_output.obj
)

add_dependencies(kernel32_shared psdk)

list(APPEND SOURCE
    client/actctx.c
    client/appcache.c
    client/atom.c
    client/compname.c
    client/debugger.c
    client/dosdev.c
    client/dllmain.c
    client/environ.c
    client/except.c
    client/fiber.c
    client/fls.c
    client/handle.c
    client/heapmem.c
    client/job.c
    client/loader.c
    client/path.c
    client/perfcnt.c
    client/power.c
    client/proc.c
    client/resntfy.c
    client/session.c
    client/synch.c
    client/sysinfo.c
    client/time.c
    client/timerqueue.c
    client/toolhelp.c
    client/utils.c
    client/thread.c
    client/vdm.c
    client/version.c
    client/virtmem.c
    client/console/alias.c
    client/console/console.c
    client/console/history.c
    client/console/init.c
    client/console/readwrite.c
    client/console/vista.c
    client/file/backup.c
    client/file/cnotify.c
    client/file/copy.c
    client/file/create.c
    client/file/delete.c
    client/file/deviceio.c
    client/file/dir.c
    client/file/disk.c
    client/file/fileinfo.c
    client/file/filemap.c
    client/file/filename.c
    client/file/find.c
    client/file/hardlink.c
    client/file/iocompl.c
    client/file/lfile.c
    client/file/lock.c
    client/file/mailslot.c
    client/file/move.c
    client/file/mntpoint.c
    client/file/npipe.c
    client/file/rw.c
    client/file/tape.c
    client/file/volume.c
    wine/actctx.c
    wine/comm.c
    wine/lzexpand.c
    wine/muldiv.c
    wine/profile.c
    wine/res.c
    wine/timezone.c
    winnls/string/casemap.c
    winnls/string/collation.c
    winnls/string/digitmap.c
    winnls/string/fold.c
    winnls/string/format_msg.c
    winnls/string/japanese.c
    winnls/string/lang.c
    winnls/string/lcformat.c
    winnls/string/lstring.c
    winnls/string/nls.c
    winnls/string/sortkey.c
    k32.h)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        client/i386/fiber.S
        client/i386/thread.S)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE
        client/amd64/fiber.S)
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE
        client/arm/fiber.S
        client/arm/thread.S)
endif()

add_asm_files(kernel32_asm ${ASM_SOURCE})

add_library(kernel32 MODULE
    ${SOURCE}
    ${kernel32_asm}
    kernel32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kernel32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/kernel32.def)

_add_library(libkernel32_vista ALIAS libkernel32)

set_module_type(kernel32 win32dll ENTRYPOINT DllMain 12)
#############################################
## HACK FOR MSVC COMPILATION WITH win32dll ##
set_subsystem(kernel32 console)
################# END  HACK #################

target_link_libraries(kernel32 kernel32_shared kernel32_vista wine chkstk ${PSEH_LIB})
add_importlibs(kernel32 ntdll advapi32)
add_pch(kernel32 k32.h SOURCE)
add_dependencies(kernel32 psdk errcodes asm)
add_cd_file(TARGET kernel32 DESTINATION reactos/system32 FOR all)
