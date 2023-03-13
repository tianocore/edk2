set(ENV{SYST_UNITTESTING} "1")
string(REPLACE "^" ";" OPTIONS ${OPTIONS})

execute_process(
    COMMAND ${EXECUTABLE} ${OPTIONS}
    OUTPUT_FILE ${TEST_OUTPUT}
    RESULT_VARIABLE EXIT_CODE
)

if(EXIT_CODE)
    message(FATAL_ERROR "execution of ${EXECUTABLE} ${OPTIONS} failed")
endif(EXIT_CODE)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E compare_files ${TEST_REFERENCE} ${TEST_OUTPUT}
    RESULT_VARIABLE DIFF_FAIL
)

if (DIFF_FAIL)
    message(FATAL_ERROR "Reference match failed.")
endif(DIFF_FAIL)