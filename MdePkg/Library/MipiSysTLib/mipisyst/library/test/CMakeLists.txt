# Google Test unit test framework must be present for tests
#
if (EXISTS "${SYST_BUILD_GTEST_DIR}/include/gtest/gtest.h")
    add_subdirectory(unit)
else()
    message("WARNING: SYST_BUILD_GTEST_DIR is not set or not pointing to Google Test sources, skipping unit tests ...")
endif ()
