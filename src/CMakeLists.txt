cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(MotionTrackingMKIIPlusforAviUtl2 CXX)

add_library(${PROJECT_NAME} SHARED)

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:_DEBUG>)

find_package(OpenCV REQUIRED)

target_sources(
    ${PROJECT_NAME}
    PRIVATE
    tracker/tracker.cpp
    ui/progress_dlg/progress_dlg.cpp
    ui/mainframe.cpp
    ui/create_frame.cpp
    ui/ownerdraw.cpp
    insert/insert_object.cpp
    filter.cpp
    main.cpp
    utils.cpp
    )

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".aux2")
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/aviutl2_sdk/include
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    comctl32
    comdlg32
    shell32
    opencv::opencv
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
)
target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)

execute_process(COMMAND git rev-list --count HEAD
    OUTPUT_VARIABLE GIT_REV
    ERROR_QUIET)
string(REPLACE "\n" "" GIT_REV "${GIT_REV}")
set(GIT_REV "#define GIT_REV \"${GIT_REV}\"")
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/config.h "${GIT_REV}")

set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
