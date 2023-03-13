include_directories(
     ${SYST_INCLUDE_DIRS}
)

add_executable(hello hello.c)
target_link_libraries(hello ${SYST_LIBRARIES})

install(TARGETS hello
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
)