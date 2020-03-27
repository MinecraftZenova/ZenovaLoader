cmake_minimum_required(VERSION 3.10)

#project(ZenovaLoader)

file(GLOB_RECURSE SOURCE_LIST
    "*.h"
    "*.cpp"
)

add_executable(ZenovaLoader WIN32
    ${SOURCE_LIST}
)

target_link_libraries(ZenovaLoader Common)