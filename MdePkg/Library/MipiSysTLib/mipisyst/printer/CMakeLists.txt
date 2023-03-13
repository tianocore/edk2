project (mipi_syst_printer)
cmake_minimum_required (VERSION 2.8)
enable_testing()

# External third party build dependencies:
# * PugiXML https://github.com/zeux/pugixml
#
if (NOT DEFINED PUGIXML_SRC_DIR)
    get_filename_component(PUGIXML "${CMAKE_CURRENT_LIST_DIR}/../external/pugixml/src" ABSOLUTE)
    set(PUGIXML_SRC_DIR "${PUGIXML}" CACHE PATH "Location of pugixml XML parser code")
endif()
if (NOT EXISTS "${PUGIXML_SRC_DIR}/pugixml.cpp")
    message(FATAL_ERROR
        "PUGIXML_SRC_DIR=${PUGIXML_SRC_DIR} does not point to pugixml sources. Try running\n"
        "git submodule update --init --recursive\n"
        "to populate the external folder git submodules or change PUGIXML_SRC_DIR to point to pugixml sources."
    )
endif()

if (WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set (printer_Includes
    include/mipi_syst_collateral.h
    include/mipi_syst_printer.h
    include/mipi_syst_decode.h
    include/mipi_syst_message.h
    include/mipi_syst_guid.h
    include/mipi_syst_printf.h

    ${PUGIXML_SRC_DIR}/pugixml.hpp
)

set (printer_Sources
    src/mipi_syst_main.cpp
    src/mipi_syst_collateral.cpp
    src/mipi_syst_printf.cpp
    src/mipi_syst_decode.cpp
    src/mipi_syst_message.cpp
    ${PUGIXML_SRC_DIR}/pugixml.cpp
)

include_directories(
    include
    ${PUGIXML_SRC_DIR}
)

add_executable(systprint
    ${printer_Includes}
    ${printer_Sources}
)

# Request C++11 support
#
if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "CLANG")
        set (CMAKE_CXX_FLAGS "-std=c++11" ${CMAKE_CXX_FLAGS})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message( STATUS ${CMAKE_CXX_COMPILER_ID})
        set (CMAKE_CXX_FLAGS "-std=gnu++11" ${CMAKE_CXX_FLAGS})
    endif()
else()
    set_property(TARGET systprint PROPERTY CXX_STANDARD 11)
endif()

install(TARGETS systprint
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Simple test that print an input file
add_test(
    NAME print_client_example
    COMMAND systprint --short_guid {494E5443-8A9C-4014-A65A-2F36A36D96E4} --collateral collateral.xml input_client64.txt
    WORKING_DIRECTORY  ${CMAKE_CURRENT_LIST_DIR}/test
)

# pass option as single "^" separated option to test script to avoid CMAKE  string list modifications on blanks
#
set(TEST_OPTIONS --short_guid {494E5443-8A9C-4014-A65A-2F36A36D96E4} --collateral collateral.xml)
string(REPLACE ";" "^" TEST_OPTIONS "${TEST_OPTIONS}")

# Compare output from 32bit client
#
add_test(
    NAME diff_output_with_32bit_reference
    COMMAND ${CMAKE_COMMAND}
        -DEXECUTABLE=$<TARGET_FILE:systprint>
        -DOPTIONS=${TEST_OPTIONS}^input_client32.txt
        -DTEST_REFERENCE=output_client32.txt
        -DTEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/output32.txt
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test/diff.cmake
    WORKING_DIRECTORY  ${CMAKE_CURRENT_LIST_DIR}/test
)

# Compare output from 64bit client
#
add_test(
    NAME diff_output_with_64bit_reference
    COMMAND ${CMAKE_COMMAND}
        -DEXECUTABLE=$<TARGET_FILE:systprint>
        -DOPTIONS=${TEST_OPTIONS}^input_client64.txt
        -DTEST_REFERENCE=output_client64.txt
        -DTEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/output64.txt
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test/diff.cmake
    WORKING_DIRECTORY  ${CMAKE_CURRENT_LIST_DIR}/test
)
# Add verbose test target to show the output of the tools run by tests
#
if (CMAKE_CONFIGURATION_TYPES)
    add_custom_target(RUN_TEST_VERBOSE COMMAND ${CMAKE_CTEST_COMMAND}
        --force-new-ctest-process --verbose
        --build-config "$<CONFIGURATION>")
else()
    add_custom_target(RUN_TEST_VERBOSE COMMAND ${CMAKE_CTEST_COMMAND}
        --force-new-ctest-process --verbose)
endif()

