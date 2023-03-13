
if (SYST_BUILD_DOC)
   if (NOT DOXYGEN_FOUND)
        message(
            WARNING
            "SYST_BUILD_DOC variable set, but Doxygen was not found -skipping documentation"
        )
   else()
         configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../include/mipi_syst.h.in
             ${CMAKE_CURRENT_BINARY_DIR}/gensrc/mipi_syst.h @ONLY
         )
         configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
             ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY
         )
 
         add_custom_target(doc
             ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
             COMMENT "Generating Doxygen manual for ${CMAKE_PROJECT_NAME} in ${CMAKE_CURRENT_BINARY_DIR}"
             SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${DOXYGEN_IDE_FILES}
           )

         set_target_properties(doc PROPERTIES
             FOLDER "Instrumentation Library"
         )
        endif()
endif(SYST_BUILD_DOC)
