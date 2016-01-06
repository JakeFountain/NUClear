##############
# GENERATORS #
###############

# XCode support
IF(CMAKE_GENERATOR MATCHES Xcode)
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++14")
ENDIF()

###############
#  COMPILERS  #
###############

# GNU Compiler
IF(CMAKE_CXX_COMPILER_ID MATCHES GNU)

    # GCC Must be version 4.8 or greater for used features
    IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8")
        MESSAGE(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.8 or greater.")
    ENDIF()

    # Enable colours on g++ 4.9 or greater
    IF(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "4.9" OR GCC_VERSION VERSION_EQUAL "4.9")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
    ENDIF()

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread -Wall -Wpedantic")

# Clang Compiler
ELSEIF(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wpedantic -Wextra")

# MSVC Compiler
ELSEIF(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    # We need features from Windows >= Vista
    MESSAGE("WINDOWS!!!")

ELSE()
    MESSAGE(FATAL_ERROR "Unsupported compiler!")
ENDIF()