#make install of googletest a nop
#
function(install)
endfunction()

enable_testing()

add_definitions(-D_VARIADIC_MAX=10)
if (WIN32)
    set (gtest_force_shared_crt ON CACHE BOOL "Force GTest to use shared CRT")
endif (WIN32)
add_subdirectory(${SYST_BUILD_GTEST_DIR} ${CMAKE_CURRENT_BINARY_DIR}/gtest)
mark_as_advanced(BUILD_SHARED_LIBS)

include_directories(
    ${SYST_BUILD_GTEST_DIR}/include
    ${mipi_syst_Platform_include}
    ${mipi_syst_Includes}
    ../../platform/src
    ../../platform/${SYST_BUILD_PLATFORM_NAME}/src
)

add_executable(syst_unittest
    mipi_syst_gtest_main.cpp
    mipi_syst_init_test.cpp
    mipi_syst_disable_test.cpp
    mipi_syst_clock_test.cpp
    mipi_syst_crc32_test.cpp
    mipi_syst_catid32_test.cpp
    mipi_syst_catid64_test.cpp
    mipi_syst_sbd_test.cpp
    mipi_syst_printf_test.cpp
    mipi_syst_raw_test.cpp
    mipi_syst_string_test.cpp
    mipi_syst_build_test.cpp
)

target_link_libraries(syst_unittest gtest mipi_syst_ut)
set_target_properties(syst_unittest  PROPERTIES COMPILE_FLAGS "-DMIPI_SYST_EXPORTS -DMIPI_SYST_UNIT_TEST")

foreach (target gtest gtest_main syst_unittest)
    set_property(TARGET ${target} PROPERTY FOLDER "Unittests")
endforeach (target)

add_test(
    NAME syst_unittest
    COMMAND syst_unittest
    WORKING_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}
)


if (CMAKE_CONFIGURATION_TYPES)
    add_custom_target(RUN_TEST_VERBOSE COMMAND ${CMAKE_CTEST_COMMAND}
        --force-new-ctest-process --verbose
        --build-config "$<CONFIGURATION>")
else()
    add_custom_target(RUN_TEST_VERBOSE COMMAND ${CMAKE_CTEST_COMMAND}
        --force-new-ctest-process --verbose)
endif()