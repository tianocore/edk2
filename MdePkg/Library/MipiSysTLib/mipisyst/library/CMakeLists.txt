project (mipi_syst_library)
cmake_minimum_required (VERSION 2.8)
enable_testing()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "")

find_package(Doxygen)

if (NOT DEFINED SYST_CFG_CONFORMANCE_LEVEL)
    message(WARNING "SYST_CFG_CONFORMANCE_LEVEL not set, defaulting to leval 30 (complete implementation)")
else()
    message(STATUS "Building for SyS-T conformance level ${SYST_CFG_CONFORMANCE_LEVEL}.")
endif()
# SYS-T API version settings
#
set(SYST_CFG_VERSION_MAJOR "1" CACHE STRING "Supported MIPI SyS-T major specification version.")
set(SYST_CFG_VERSION_MINOR "0" CACHE STRING "Supported MIPI SyS-T minor specification version.")
set(SYST_CFG_CONFORMANCE_LEVEL "30" CACHE STRING "Supported MIPI SyS-T API conformance level. (10=min, 20=low overhead, 30=complete)")

# SYS-T Implementation patch level
#
set(SYST_CFG_VERSION_PATCH "0" CACHE PATH "SyS-T Library patch level.")



# External build dependencies:
# * Google Test https://github.com/google/googletest (for unit tests)
#
if (NOT DEFINED SYST_BUILD_GTEST_DIR)
    get_filename_component(GTEST_DIR "${CMAKE_CURRENT_LIST_DIR}/../external/googletest/googletest" ABSOLUTE)
endif()
set(SYST_BUILD_GTEST_DIR "${GTEST_DIR}" CACHE PATH "Location of Google Test sources, which are needed for unit tests")
if (NOT EXISTS  "${SYST_BUILD_GTEST_DIR}/include/gtest/gtest.h")
    message(WARNING
        "SYST_BUILD_GTEST_DIR=${SYST_BUILD_GTEST_DIR} does not point to a Google Test source location. Unittests will not be build. Try running \n"
        "git submodule update --init --recursive\n"
        "to populate the external folder git submodules or change SYST_BUILD_GTEST_DIR to point to Google Test sources.")
    set(BUILD_TEST "OFF")
else()
    set(BUILD_TEST "ON")
endif()

option(SYST_BUILD_DOC "Create documentation (requires Doxygen)" ${DOXYGEN_FOUND})
option(SYST_BUILD_TEST "Build unit test (requires GoogleTest)" ${BUILD_TEST})
option(SYST_BUILD_HW_CRC "Use CPU crc32 instruction support if supported by architecture." ON)

if (NOT DEFINED SYST_BUILD_PLATFORM_NAME)
    set (SYST_BUILD_PLATFORM_NAME "example" CACHE STRING "Directory name of platform specific source code")
    message(WARNING "SYST_BUILD_PLATFORM_NAME directory name  variable is not set, using 'example' as default.")
endif()


# HW CRC support enabling
#
if (SYST_BUILD_HW_CRC)
   if (CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86|x86_64.*|i?86.*|AMD64.*")
        add_definitions(-D MIPI_SYST_CRC_INTRINSIC_ON)
        message(STATUS "Enabling x86 crc32 CPU instruction support (SSE 4.2). Unset SYST_BUILD_HW_CRC to disable." )
        if (NOT WIN32)
            add_definitions(-msse4.2)
        endif()
    endif()
endif()

configure_file(include/mipi_syst.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/mipi_syst.h)

set (mipi_syst_Includes
     include/mipi_syst.h.in
     include/mipi_syst/api.h
     include/mipi_syst/compiler.h
     include/mipi_syst/crc32.h
     include/mipi_syst/message.h
     include/mipi_syst/inline.h
)

set (mipi_syst_Sources
     src/mipi_syst_api.c
     src/mipi_syst_compiler.c
     src/mipi_syst_crc32.c
     src/mipi_syst_init.c
     src/mipi_syst_inline.c
     src/mipi_syst_writer.c
)

set (mipi_syst_Platform_inc          platform/${SYST_BUILD_PLATFORM_NAME}/include)
set (mipi_syst_Platform_src          platform/${SYST_BUILD_PLATFORM_NAME}/src/mipi_syst_platform.c)

file(GLOB mipi_syst_Platform_includes "${mipi_syst_Platform_inc}/*.h")

include_directories(
     ${mipi_syst_Platform_inc}
     ${CMAKE_CURRENT_BINARY_DIR}/include
     include
)

add_library(mipi_syst
         SHARED
         ${mipi_syst_Platform_includes}
         ${mipi_syst_Includes}
         ${mipi_syst_Sources}
         ${mipi_syst_Platform_src}
)

add_library(mipi_syst_static
         STATIC
         ${mipi_syst_Platform_includes}
         ${mipi_syst_Includes}
         ${mipi_syst_Sources}
         ${mipi_syst_Platform_src}
)

add_library(mipi_syst_ut
         STATIC
         ${mipi_syst_Platform_includes}
         ${mipi_syst_Includes}
         ${mipi_syst_Sources}
         ${mipi_syst_Platform_src}
)

set_target_properties(mipi_syst_ut  PROPERTIES
    VERSION ${SYST_CFG_VERSION_MAJOR}.${SYST_CFG_VERSION_MINOR}.${SYST_CFG_VERSION_PATCH}
    COMPILE_FLAGS "-DMIPI_SYST_STATIC -DMIPI_SYST_UNIT_TEST"
    FOLDER "Instrumentation Library"
)

set_target_properties(mipi_syst  PROPERTIES
    VERSION ${SYST_CFG_VERSION_MAJOR}.${SYST_CFG_VERSION_MINOR}.${SYST_CFG_VERSION_PATCH}
    COMPILE_FLAGS "-DMIPI_SYST_EXPORTS"
    FOLDER "Instrumentation Library"
)

set_target_properties(mipi_syst_static  PROPERTIES
    VERSION ${SYST_CFG_VERSION_MAJOR}.${SYST_CFG_VERSION_MINOR}.${SYST_CFG_VERSION_PATCH}
    COMPILE_FLAGS "-DMIPI_SYST_STATIC"
    FOLDER "Instrumentation Library"
)

install(TARGETS mipi_syst mipi_syst_static
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/mipi_syst DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/ DESTINATION include)
install(DIRECTORY ${mipi_syst_Platform_inc}/ DESTINATION include)

add_subdirectory(doxygen)
add_subdirectory(test)
