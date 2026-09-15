function(tavoos_add_resources TARGET)
    foreach(RESOURCE_PATH ${ARGN})
        get_filename_component(ABS_PATH "${RESOURCE_PATH}" ABSOLUTE)
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