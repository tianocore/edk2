project(SYST_Examples)
cmake_minimum_required (VERSION 2.8)

get_filename_component(
        THIS_PATH "${CMAKE_CURRENT_SOURCE_DIR}"
        ABSOLUTE
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${THIS_PATH}/cmake")

if (defined $ENV{SYST_SDK})
    set(SYST_SDK "ENV{SYST_SDK}" CACHE PATH "SYS-T header and library root folder")
else()
    set(SYST_SDK "(not-set)" CACHE PATH "SYS-T header and library root folder")
endif()

find_package(SyST REQUIRED)

# static linking flags
#
add_definitions(-DMIPI_SYST_STATIC)
set(SYST_LIBRARIES ${SYST_LIBRARIES_STATIC})

#dynamic linking, requires location of mipi_syst.dll to be in the PATH.
#set(SYST_LIBRARIES ${SYST_LIBRARIES_DYNAMIC})

add_subdirectory(hello)
add_subdirectory(client)