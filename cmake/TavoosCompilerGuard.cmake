if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14)
    message(FATAL_ERROR "Tavoos uses C++23 'deducing this' (its fluent widget-builder API), which needs GCC >= 14 - found ${CMAKE_CXX_COMPILER_VERSION}. Install a newer GCC, or point CMAKE_CXX_COMPILER at one, and reconfigure.")
endif()
