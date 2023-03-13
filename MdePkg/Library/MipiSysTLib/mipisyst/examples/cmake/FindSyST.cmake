# - Try to find the SyS-T headers and libraries
#
# This module defines
# SYST_INCLUDE_DIRS, where to find header files
# SYST_LIBRARIES,   the libraries to link against
# SYST_FOUND, true if SyS-T installation found


if (NOT SYST_SDK AND NOT $ENV{SYST_SDK} STREQUAL "")
    set(SYST_SDK $ENV{SYST_SDK})
endif()

set (SYST_INCLUDE_SEARCH_DIRS
        ../include
        /usr/include
        /usr/local/include
)

set (SYST_LIBRARIES_SEARCH_DIRS
        ../lib
        /usr/lib
        /usr/local/lib
)

if (SYST_SDK)
    file(TO_CMAKE_PATH ${SYST_SDK} SYST_SDK)


    set (SYST_INCLUDE_SEARCH_DIRS
            ${SYST_SDK}/include
            ${SYST_INCLUDE_SEARCH_DIRS}
    )

    set (SYST_LIBRARIES_SEARCH_DIRS
        ${SYST_SDK}/bin
        ${SYST_SDK}/lib
        ${SYST_INCLUDE_SEARCH_DIRS}
    )
endif ()

find_path (SYST_INCLUDE_DIRS
               mipi_syst.h
           HINTS
               ${SYST_INCLUDE_SEARCH_DIRS}
)

find_library(SYST_LIBRARIES_STATIC
                mipi_syst_static
              HINTS
                ${SYST_LIBRARIES_SEARCH_DIRS}
)

find_library(SYST_LIBRARIES_DYNAMIC
                mipi_syst
              HINTS
                ${SYST_LIBRARIES_SEARCH_DIRS}
)


include(FindPackageHandleStandardArgs)

set (SYST_notfound_msg
         "Could not find MIPI SyS-T SDK. Try setting SYST environment variable."
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS (
      SYST DEFAULT_MSG SYST_INCLUDE_DIRS SYST_LIBRARIES_STATIC SYST_LIBRARIES_DYNAMIC
)

mark_as_advanced (
        SYST_INCLUDE_DIRS
        SYST_LIBRARIES_STATIC
        SYST_LIBRARIES_DYNAMIC
)
