# IDL files support - WIDL

if(ARCH STREQUAL "i386")
    set(IDL_FLAGS -m32 --win32)
elseif(ARCH STREQUAL "amd64")
    set(IDL_FLAGS -m64 --win64)
else()
    set(IDL_FLAGS "")
endif()

include(sdk/cmake/idl-support.cmake)

function(add_typelib)
    get_includes(INCLUDES)
    get_defines(DEFINES)

    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)

        if(${NAME} STREQUAL "std_ole_v1")
            set(IDL_FLAGS ${IDL_FLAGS} --oldtlb)
        endif()

        convert_path_to_absolute_and_relative(${FILE} IDL_FILE_ABSOLUTE IDL_FILE_RELATIVE)

        set(TLB_FILE ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.tlb)

        add_custom_command(
            OUTPUT ${TLB_FILE}
            COMMAND native-widl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} -t -T ${TLB_FILE} --tmp=${CMAKE_CURRENT_BINARY_DIR}/ ${IDL_FILE_RELATIVE}
            DEPENDS ${IDL_FILE_ABSOLUTE} native-widl
            WORKING_DIRECTORY ${REACTOS_SOURCE_DIR}
        )
        list(APPEND OBJECTS ${TLB_FILE})
    endforeach()
endfunction()

function(add_idl_headers TARGET)
    get_includes(INCLUDES)
    get_defines(DEFINES)

    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)

        convert_path_to_absolute_and_relative(${FILE} IDL_FILE_ABSOLUTE IDL_FILE_RELATIVE)

        set(HEADER ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.h)

        add_custom_command(
            OUTPUT ${HEADER}
            COMMAND native-widl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} -h -H ${HEADER} --tmp=${CMAKE_CURRENT_BINARY_DIR}/ ${IDL_FILE_RELATIVE}
            DEPENDS ${IDL_FILE_ABSOLUTE} native-widl
            WORKING_DIRECTORY ${REACTOS_SOURCE_DIR}
        )

        list(APPEND HEADERS ${HEADER})
    endforeach()

    add_custom_target(${TARGET} DEPENDS ${HEADERS})
endfunction()

function(add_rpcproxy_files)
    get_includes(INCLUDES)
    get_defines(DEFINES)

    set(PROXY_DLLDATA_FILE ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c)

    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)

        convert_path_to_absolute_and_relative(${FILE} IDL_FILE_ABSOLUTE IDL_FILE_RELATIVE)

        set(PROXY_C_FILE ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_p.c)
        set(PROXY_H_FILE ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_p.h)

        list(APPEND IDLS ${IDL_FILE_ABSOLUTE})
        list(APPEND PROXY_FILES ${PROXY_C_FILE} ${PROXY_H_FILE})

        # Most proxy idl's have names like <proxyname>_<original>.idl
        # We use this to create a dependency from the proxy to the original idl
        string(REPLACE "_" ";" SPLIT_FILE ${FILE})
        list(LENGTH SPLIT_FILE len)
        if(len STREQUAL "2")
            list(GET SPLIT_FILE 1 SPLIT_FILE)
            if(EXISTS "${REACTOS_SOURCE_DIR}/sdk/include/psdk/${SPLIT_FILE}")
                list(APPEND IDL_EXTRA_DEPS "${REACTOS_SOURCE_DIR}/sdk/include/psdk/${SPLIT_FILE}")
            endif()
        endif()

        add_custom_command(
            OUTPUT ${PROXY_C_FILE} ${PROXY_H_FILE}
            COMMAND native-widl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} -p -h -P ${PROXY_C_FILE} -H ${PROXY_H_FILE} --dlldata ${PROXY_DLLDATA_FILE} --tmp=${CMAKE_CURRENT_BINARY_DIR}/ ${IDL_FILE_RELATIVE}
            DEPENDS ${IDLS} ${IDL_EXTRA_DEPS} native-widl
            WORKING_DIRECTORY ${REACTOS_SOURCE_DIR}
        )
    endforeach()

    set_source_files_properties(${PROXY_DLLDATA_FILE} PROPERTIES GENERATED TRUE)
    set_source_files_properties(${PROXY_DLLDATA_FILE} PROPERTIES OBJECT_DEPENDS "${IDLS};${IDL_EXTRA_DEPS};${PROXY_FILES}")
endfunction()

function(add_rpc_files __type)
    get_includes(INCLUDES)
    get_defines(DEFINES)

    set(__additional_flags -Oif)

    # Is it a client or server module?
    if(__type STREQUAL "server")
        set(__server_client s)
        set(__server_client_upcase S)
    elseif(__type STREQUAL "client")
        set(__server_client c)
        set(__server_client_upcase C)
    else()
        message(FATAL_ERROR "Please pass either server or client as argument to add_rpc_files")
    endif()

    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)

        convert_path_to_absolute_and_relative(${FILE} IDL_FILE_ABSOLUTE IDL_FILE_RELATIVE)

        set(C_FILE ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_${__server_client}.c)
        set(H_FILE ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_${__server_client}.h)

        add_custom_command(
            OUTPUT ${C_FILE} ${H_FILE}
            COMMAND native-widl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} ${__additional_flags} -${__server_client} -${__server_client_upcase} ${C_FILE} -h -H ${H_FILE} --tmp=${CMAKE_CURRENT_BINARY_DIR}/ ${IDL_FILE_RELATIVE}
            DEPENDS ${IDL_FILE_ABSOLUTE} native-widl
            WORKING_DIRECTORY ${REACTOS_SOURCE_DIR}
        )
    endforeach()
endfunction()

function(generate_idl_iids)
    get_includes(INCLUDES)
    get_defines(DEFINES)

    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)

        convert_path_to_absolute_and_relative(${FILE} IDL_FILE_ABSOLUTE IDL_FILE_RELATIVE)

        set(IID_C_FILE ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_i.c)

        add_custom_command(
            OUTPUT ${IID_C_FILE}
            COMMAND native-widl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} -u -U ${IID_C_FILE} --tmp=${CMAKE_CURRENT_BINARY_DIR}/ ${IDL_FILE_RELATIVE}
            DEPENDS ${IDL_FILE_ABSOLUTE} native-widl
            WORKING_DIRECTORY ${REACTOS_SOURCE_DIR}
        )
    endforeach()
endfunction()

function(add_iid_library TARGET)
    foreach(IDL_FILE ${ARGN})
        get_filename_component(NAME ${IDL_FILE} NAME_WE)
        generate_idl_iids(${IDL_FILE})
        list(APPEND IID_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_i.c)
    endforeach()

    add_library(${TARGET} ${IID_SOURCES})

    # for wtypes.h
    add_dependencies(${TARGET} psdk)

    set_target_properties(${TARGET} PROPERTIES EXCLUDE_FROM_ALL TRUE)
endfunction()
