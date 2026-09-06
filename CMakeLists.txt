cmake_minimum_required(VERSION 3.18)

if(POLICY CMP0177)
    cmake_policy(SET CMP0177 NEW)
endif()

project(XHexViewer)

include(CTest)
add_subdirectory(src)

if(BUILD_TESTING AND NOT WIN32)
    add_test(NAME xhexviewer.version COMMAND $<TARGET_FILE:XHexViewer> --version)
    set_tests_properties(xhexviewer.version PROPERTIES
        PASS_REGULAR_EXPRESSION "XHexViewer v"
    )
endif()
