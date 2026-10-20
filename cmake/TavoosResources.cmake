function(tavoos_add_resources TARGET)
    cmake_parse_arguments(ARG "" "BASE_DIR" "" ${ARGN})
    if (ARG_BASE_DIR)
        set(BASE_DIR "${ARG_BASE_DIR}")
    else()
        set(BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    foreach(RESOURCE_PATH ${ARG_UNPARSED_ARGUMENTS})
        get_filename_component(ABS_PATH "${RESOURCE_PATH}" ABSOLUTE BASE_DIR "${BASE_DIR}")
        string(MD5 SYMBOL "${RESOURCE_PATH}")
        set(OUTPUT_CPP "${CMAKE_CURRENT_BINARY_DIR}/tavoos_resources/res_${SYMBOL}.cpp")

        add_custom_command(
            OUTPUT "${OUTPUT_CPP}"
            COMMAND ${CMAKE_COMMAND}
                -DINPUT_FILE=${ABS_PATH}
                -DOUTPUT_FILE=${OUTPUT_CPP}
                -DRESOURCE_PATH=${RESOURCE_PATH}
                -DSYMBOL=${SYMBOL}
                -P ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/GenerateResource.cmake
            DEPENDS "${ABS_PATH}" "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/GenerateResource.cmake"
            COMMENT "Embedding resource: ${RESOURCE_PATH}"
            VERBATIM
        )
        target_sources(${TARGET} PRIVATE "${OUTPUT_CPP}")
    endforeach()
endfunction()