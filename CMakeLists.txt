
cmake_minimum_required(VERSION 3.23)
project(1brc)

add_executable(
    1brc
    src/main.cpp
    )

target_compile_features(
    1brc
    PRIVATE
    cxx_std_17
    )

find_package(Threads)
target_link_libraries(
    1brc
    PRIVATE
    ${CMAKE_THREAD_LIBS_INIT}
    )


