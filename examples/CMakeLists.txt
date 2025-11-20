cmake_minimum_required(VERSION 3.20)

# Example programs
add_executable(simple_viewer simple_viewer.cpp)
target_link_libraries(simple_viewer PRIVATE XtionDriver)

add_executable(depth_recorder depth_recorder.cpp)
target_link_libraries(depth_recorder PRIVATE XtionDriver)

add_executable(all_streams all_streams.cpp)
target_link_libraries(all_streams PRIVATE XtionDriver)

# Installation
install(TARGETS simple_viewer depth_recorder all_streams
    RUNTIME DESTINATION bin/examples
)
