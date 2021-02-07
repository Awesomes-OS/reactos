remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

add_definitions(-D_KERNEL32_)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)

list(APPEND SOURCE
    # DllMain.c
    GetFileInformationByHandleEx.c
    GetTickCount64.c
    InitOnceExecuteOnce.c
    sync.c
    vista.c
    threadpool.c
    wer.c
)

add_library(kernel32_vista OBJECT ${SOURCE})
add_dependencies(kernel32_vista psdk)
