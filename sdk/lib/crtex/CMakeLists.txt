add_subdirectory(mingw)
add_subdirectory(ms)

add_library(libcrtex STATIC)
target_link_libraries(libcrtex PRIVATE libcrtex_mingw libcrtex_ms)
