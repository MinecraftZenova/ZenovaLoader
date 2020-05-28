cmake_minimum_required(VERSION 3.10)
set(project_name "ZenovaLoader")

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

project(${project_name})

file(GLOB_RECURSE SOURCE_LIST
    "*.h"
    "*.cpp"
)

add_executable(${project_name} WIN32
    ${SOURCE_LIST}
)

set_target_properties(
    ${project_name} 
    PROPERTIES
        SUFFIX ""
)

target_link_libraries(${project_name} Common)