cmake_minimum_required(VERSION 3.20)

# Test programs
add_executable(test_usb_enum test_usb_enum.cpp)
target_link_libraries(test_usb_enum PRIVATE XtionDriver)

add_executable(test_device_open test_device_open.cpp)
target_link_libraries(test_device_open PRIVATE XtionDriver)

add_executable(test_depth_stream test_depth_stream.cpp)
target_link_libraries(test_depth_stream PRIVATE XtionDriver)

# Installation
install(TARGETS test_usb_enum test_device_open test_depth_stream
    RUNTIME DESTINATION bin/tests
)
