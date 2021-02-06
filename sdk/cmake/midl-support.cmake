# IDL files support - MIDL

set(IDL_FLAGS /nologo /target NT60 /robust /no_def_idir)

if(ARCH STREQUAL "i386")
    set(IDL_FLAGS ${IDL_FLAGS} /win32)
elseif(ARCH STREQUAL "amd64")
    set(IDL_FLAGS ${IDL_FLAGS} /amd64)
else()
    message(FATAL_ERROR "Unsupported MIDL architecture.")
endif()

include(sdk/cmake/idl-support.cmake)

function(add_typelib)
    get_includes(INCLUDES)
    get_defines(DEFINES)

    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)

        if(${NAME} STREQUAL "std_ole_v1")
            set(IDL_FLAGS ${IDL_FLAGS} /oldtlb)
        endif()

        convert_path_to_absolute_and_relative(${FILE} IDL_FILE_ABSOLUTE IDL_FILE_RELATIVE)

        set(TLB_FILE ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.tlb)

        add_custom_command(
            OUTPUT ${TLB_FILE}
            COMMAND midl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} /tlb ${TLB_FILE} ${IDL_FILE_RELATIVE}
            DEPENDS ${IDL_FILE_ABSOLUTE}
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

        set(IDL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${FILE})
        set(HEADER ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.h)
        set(DUMMY_IID ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_dummy_i.c)

        add_custom_command(
            OUTPUT ${HEADER}
            COMMAND midl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} /h ${HEADER} /client none /server none /iid ${DUMMY_IID} ${IDL_FILE_RELATIVE}
            DEPENDS ${IDL_FILE_ABSOLUTE}
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
            COMMAND midl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} /client none /server none /proxy ${PROXY_C_FILE} /h ${PROXY_H_FILE} /dlldata ${PROXY_DLLDATA_FILE} ${IDL_FILE_RELATIVE}
            DEPENDS ${IDLS} ${IDL_EXTRA_DEPS}
            WORKING_DIRECTORY ${REACTOS_SOURCE_DIR}
        )
    endforeach()

    set_source_files_properties(${PROXY_DLLDATA_FILE} PROPERTIES GENERATED TRUE)
    set_source_files_properties(${PROXY_DLLDATA_FILE} PROPERTIES OBJECT_DEPENDS "${IDLS};${IDL_EXTRA_DEPS};${PROXY_FILES}")
endfunction()

function(add_rpc_files __type)
    get_includes(INCLUDES)
    get_defines(DEFINES)

    # Is it a client or server module?
    if(__type STREQUAL "server")
        set(__server_client s)
        set(__prevent_second_type /client none)
    elseif(__type STREQUAL "client")
        set(__server_client c)
        set(__prevent_second_type /server none)
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
            COMMAND midl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} /h ${H_FILE} /${__server_client}stub ${C_FILE} ${__prevent_second_type} ${IDL_FILE_RELATIVE}
            DEPENDS ${IDL_FILE_ABSOLUTE}
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
        set(IID_H_FILE ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_i.h)
        # set(IID_PROXY_FILE ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_dummy_p.c)

        add_custom_command(
            OUTPUT ${IID_C_FILE} ${IID_H_FILE}
            # COMMAND midl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} /h ${IID_H_FILE} /client none /server none /iid ${IID_C_FILE} /proxy ${IID_PROXY_FILE} ${IDL_FILE_RELATIVE}
            COMMAND midl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} /h ${IID_H_FILE} /client none /server none /iid ${IID_C_FILE} ${IDL_FILE_RELATIVE}
            DEPENDS ${IDL_FILE_ABSOLUTE}
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
