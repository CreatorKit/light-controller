# Add docs targets
#################
CMAKE_MINIMUM_REQUIRED (VERSION 2.6.2)

FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
  SET(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in)

  SET(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/doxyfile)
  CONFIGURE_FILE(${doxyfile_in} ${doxyfile} @ONLY)

  ADD_CUSTOM_TARGET(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
ELSE()
  MESSAGE(WARNING "Doxygen not found. Cannot generate documentation!")
  ADD_CUSTOM_TARGET(docs
    COMMENT "***Doxygen not found! Please install doxygen to make docs***"
    VERBATIM)
ENDIF()
