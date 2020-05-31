cmake_minimum_required(VERSION 3.10)
set(project_name "ZenovaLoader")

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(${project_name})

# need to switch this so that the Zenova all in one build still works if used
if(EXISTS "${PROJECT_SOURCE_DIR}/Common")
    add_subdirectory("${PROJECT_SOURCE_DIR}/Common")
endif()

file(GLOB_RECURSE SOURCE_LIST
    "src/ZenovaLoader.rc"
    "src/*.h"
    "src/*.cpp"
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