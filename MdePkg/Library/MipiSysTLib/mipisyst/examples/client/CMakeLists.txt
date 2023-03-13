include_directories(
     ${SYST_INCLUDE_DIRS}
)

add_executable(systclient systclient.c othersource.c)
target_link_libraries(systclient ${SYST_LIBRARIES})

install(TARGETS systclient
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
)