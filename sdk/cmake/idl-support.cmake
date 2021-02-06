# IDL files support

function(assert_relative PATH)
    if(IS_ABSOLUTE ${PATH})
        message(FATAL_ERROR "Path '${PATH}' is expected to be relative.")
        return()
    endif()

    string(FIND "${PATH}" "/" SLASH_INDEX)
    if(SLASH_INDEX EQUAL 0)
        message(FATAL_ERROR "Path '${PATH}' is expected to not start with '/'.")
        return()
    endif()

    string(FIND "${PATH}" "\\" SLASH_INDEX)
    if(SLASH_INDEX EQUAL 0)
        message(FATAL_ERROR "Path '${PATH}' is expected to not start with '\\'.")
        return()
    endif()
endfunction()

function(assert_source_root_relative_path_exists ABSOLUTE_ORIGINAL_PATH NEW_PATH)
    assert_relative(${NEW_PATH})

    if(NOT EXISTS ${ABSOLUTE_ORIGINAL_PATH})
        return()
    endif()

    set(NEW_PATH_ABSOLUTE "${REACTOS_SOURCE_DIR}/${NEW_PATH}")

    if(NOT EXISTS ${NEW_PATH_ABSOLUTE})
        message(FATAL_ERROR "Path '${NEW_PATH_ABSOLUTE}' ('${NEW_PATH}') is expected to exist (since '${ABSOLUTE_ORIGINAL_PATH}' exists).")
        return()
    endif()
endfunction()

function(convert_absolute_path_to_relative_to_source_root PATH OUTPUT_VAR)
    if(NOT IS_ABSOLUTE ${PATH})
        message(FATAL_ERROR "Path '${PATH}' is expected to be absolute.")
        return()
    endif()

    string(FIND "${PATH}" "${REACTOS_SOURCE_DIR}" ROS_SOURCE_INDEX)
    if(ROS_SOURCE_INDEX EQUAL -1)
        message(FATAL_ERROR "Path '${PATH}' is expected to be within source root ('${REACTOS_SOURCE_DIR}').")
        return()
    endif()

    string(REPLACE "${REACTOS_SOURCE_DIR}" "" RESULT "${PATH}")

    if(NOT ${RESULT} MATCHES "^[/\\][^/\\]+.*")
        message(FATAL_ERROR "Path '${RESULT}' is expected to start with a single directory separator.")
        return()
    endif()

    string(SUBSTRING "${RESULT}" 1 -1 RESULT)

    assert_source_root_relative_path_exists(${PATH} ${RESULT})
    set(${OUTPUT_VAR} "${RESULT}" PARENT_SCOPE)
endfunction()

function(convert_relative_path_to_relative_to_source_root PATH OUTPUT_VAR)
    assert_relative(${PATH})

    set(PATH_FROM_ROOT "${REACTOS_SOURCE_DIR}/${PATH}")

    if(EXISTS ${PATH_FROM_ROOT})
        set(${OUTPUT_VAR} "${PATH_FROM_ROOT}" PARENT_SCOPE)
        return()
    endif()

    set(PATH_FROM_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${PATH}")

    if(EXISTS ${PATH_FROM_SOURCE})
        convert_absolute_path_to_relative_to_source_root(${PATH_FROM_SOURCE} RESULT)
        set(${OUTPUT_VAR} "${RESULT}" PARENT_SCOPE)
        return()
    endif()

    message(FATAL_ERROR "Path '${PATH}' is in an unsupported location.")
endfunction()

function(convert_path_to_absolute_and_relative FILE OUTPUT_ABSOLUTE_VAR OUTPUT_RELATIVE_VAR)
    if(IS_ABSOLUTE ${FILE})
        set(${OUTPUT_ABSOLUTE_VAR} "${FILE}" PARENT_SCOPE)
        convert_absolute_path_to_relative_to_source_root(${FILE} RELATIVE)
    else()
        convert_relative_path_to_relative_to_source_root(${FILE} RELATIVE)
        set(${OUTPUT_ABSOLUTE_VAR} "${REACTOS_SOURCE_DIR}/${RELATIVE}" PARENT_SCOPE)
    endif()

    set(${OUTPUT_RELATIVE_VAR} "${RELATIVE}" PARENT_SCOPE)
endfunction()
