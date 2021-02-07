
add_subdirectory(bootsect)

if (NOT NTDLL_WORK)
add_subdirectory(fdebug)
endif()

add_subdirectory(freeldr)
add_subdirectory(install)
