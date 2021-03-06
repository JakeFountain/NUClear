CMAKE_MINIMUM_REQUIRED(VERSION 2.8.10)
PROJECT(NUClear)

SET(VERSION_MAJOR 0)
SET(VERSION_MINOR 2)
SET(VERSION_PATCH 0)

# Supported options:
OPTION(BUILD_TESTS "Builds all of the NUClear unit tests." ON)

# We use additional modules that cmake needs to know about
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# This directory contains our header files and ipp files
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src/include)

# Setup our compiler settings
INCLUDE(CompilerSetup)

# Add the subdirectories
ADD_SUBDIRECTORY(src)

# Install the headers and templates that nuclear depends on
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/include/ DESTINATION include PATTERN "*.hpp" PATTERN "*.ipp")

# Optional build supports
IF(BUILD_TESTS)
    ENABLE_TESTING()
    ADD_SUBDIRECTORY(tests)
ENDIF()
