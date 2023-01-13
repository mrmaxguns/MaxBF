cmake_minimum_required(VERSION 3.1)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(MaxBF VERSION 1.0
              DESCRIPTION "A Brainfuck interpreter written in C."
              LANGUAGES C)

add_executable(maxbf maxbf.c)
set_property(TARGET maxbf
             PROPERTY C_STANDARD 99)

add_executable(test_maxbf test_maxbf.c)
target_compile_definitions(test_maxbf PUBLIC -DTESTING)
add_test(NAME TestMaxBF
         COMMAND test_maxbf)
enable_testing()
