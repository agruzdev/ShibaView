#
# ShibaView
#
# Apache License, Version 2.0
# Copyright 2016-2020 Alexey Gruzdev
#

cmake_minimum_required(VERSION 3.0)

if(DEFINED TOOLCHAIN_USED)
    return()
endif()
set(TOOLCHAIN_USED TRUE)

if(CMAKE_TOOLCHAIN_FILE)
    # touch toolchain variable to suppress "unused variable" warning
endif()

set(CMAKE_C_COMPILER   "/usr/bin/x86_64-w64-mingw32-gcc-posix" CACHE STRING "C compiler")
set(CMAKE_CXX_COMPILER "/usr/bin/x86_64-w64-mingw32-g++-posix" CACHE STRING "C++ compiler")


#set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${target}" CACHE STRING "C flags")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${target} -std=c++14 " CACHE STRING "C++ flags")
#set(CMAKE_C_STANDARD_LIBRARIES   "${CMAKE_C_STANDARD_LIBRARIES}  -lgcc "    CACHE STRING "Standard C Libraries")
#set(CMAKE_CXX_STANDARD_LIBRARIES "${MAKE_CXX_STANDARD_LIBRARIES} -lstdc++ " CACHE STRING "Standard C++ Libraries")

#unset(target)

#set(CLANG ON)
#set(CLANG_MINGW ON)
#set(WIN32 ON)
