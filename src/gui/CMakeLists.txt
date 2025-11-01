cmake_minimum_required(VERSION 3.5)

include(../../dep/build_tools/cmake/deploy.cmake)
include(GNUInstallDirs)

project(xhexviewer VERSION ${X_PROJECT_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /MANIFEST:NO")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")
endif()

add_definitions(-DUSE_DEX)
add_definitions(-DUSE_PDF)
add_definitions(-DUSE_ARCHIVE)
add_definitions(-DUSE_YARA)

include(${CMAKE_CURRENT_LIST_DIR}/../../dep//XHexView/xhexview.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../dep/XAboutWidget/xaboutwidget.cmake)

set(TS_FILES
    ../../res/translation/xhexviewer_ar.ts
    ../../res/translation/xhexviewer_bn.ts
    ../../res/translation/xhexviewer_de.ts
)

set(PROJECT_SOURCES
        ${XHEXVIEW_SOURCES}
        ${XABOUTWIDGET_SOURCES}
        dialogabout.cpp
        dialogabout.h
        dialogabout.ui
        dialogoptions.cpp
        dialogoptions.h
        dialogoptions.ui
        main_gui.cpp
        guimainwindow.cpp
        guimainwindow.h
        guimainwindow.ui
        rsrc.qrc
)

#qt_create_translation(QM_FILES ${PROJECT_SOURCES} ${TS_FILES})
#qt_add_translation(QM_FILES ${TS_FILES})

if(WIN32)
    add_executable(xhexviewer WIN32
        ${PROJECT_SOURCES}
        ../../res/resource.rc
    )
else()
    add_executable(xhexviewer
        ${PROJECT_SOURCES}
    )
endif()

set_target_properties(xhexviewer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(xhexviewer PRIVATE bzip2)
target_link_libraries(xhexviewer PRIVATE lzma)
target_link_libraries(xhexviewer PRIVATE zlib)
target_link_libraries(xhexviewer PRIVATE ppmd)
target_link_libraries(xhexviewer PRIVATE capstone)
target_link_libraries(xhexviewer PRIVATE cppfilt)
target_link_libraries(xhexviewer PRIVATE yara)

target_link_libraries(xhexviewer PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(xhexviewer PRIVATE Qt${QT_VERSION_MAJOR}::Concurrent)
target_link_libraries(xhexviewer PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(xhexviewer PRIVATE Qt${QT_VERSION_MAJOR}::PrintSupport)
target_link_libraries(xhexviewer PRIVATE Qt${QT_VERSION_MAJOR}::OpenGL)
target_link_libraries(xhexviewer PRIVATE Qt${QT_VERSION_MAJOR}::Svg)
target_link_libraries(xhexviewer PRIVATE Qt${QT_VERSION_MAJOR}::Sql)

if(${QT_VERSION_MAJOR} EQUAL 5)
    target_link_libraries(xhexviewer PRIVATE Qt${QT_VERSION_MAJOR}::Script)
    target_link_libraries(xhexviewer PRIVATE Qt${QT_VERSION_MAJOR}::ScriptTools)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    target_link_libraries(xhexviewer PRIVATE Qt${QT_VERSION_MAJOR}::Qml)
endif()

if(WIN32)
    target_link_libraries(xhexviewer PRIVATE Wintrust)
    target_link_libraries(xhexviewer PRIVATE Crypt32)
endif()

if(WIN32)
    install (TARGETS xhexviewer DESTINATION "./")
    install (DIRECTORY ../../dep/Detect-It-Easy/db DESTINATION "./" OPTIONAL)
    install (DIRECTORY ../../dep/Detect-It-Easy/db_custom DESTINATION "./" OPTIONAL)
    install (DIRECTORY ../../dep/XInfoDB/info DESTINATION "./" OPTIONAL)
    install (DIRECTORY ../../dep/XStyles/qss DESTINATION "./" OPTIONAL)
    install (DIRECTORY ../../dep/XYara/yara_rules DESTINATION "./" OPTIONAL)
    install (FILES ../../dep/signatures/crypto.db DESTINATION "./signatures" OPTIONAL)
    deploy_qt()
    deploy_msvc()
elseif(APPLE)
    install(TARGETS xhexviewer
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
else()
    install (TARGETS xhexviewer DESTINATION ${CMAKE_INSTALL_BINDIR})
    install (DIRECTORY ../../dep/Detect-It-Easy/db DESTINATION ${CMAKE_INSTALL_LIBDIR}/xhexviewer OPTIONAL)
    install (DIRECTORY ../../dep/Detect-It-Easy/db_custom DESTINATION ${CMAKE_INSTALL_LIBDIR}/xhexviewer OPTIONAL)
    install (DIRECTORY ../../dep/XInfoDB/info DESTINATION ${CMAKE_INSTALL_LIBDIR}/xhexviewer OPTIONAL)
    install (DIRECTORY ../../dep/XStyles/qss DESTINATION ${CMAKE_INSTALL_LIBDIR}/xhexviewer OPTIONAL)
    install (DIRECTORY ../../dep/XYara/yara_rules DESTINATION ${CMAKE_INSTALL_LIBDIR}/xhexviewer OPTIONAL)
    install (FILES ../../dep/signatures/crypto.db DESTINATION ${CMAKE_INSTALL_LIBDIR}/xhexviewer/signatures OPTIONAL)
endif()

# if(${QT_VERSION_MAJOR} EQUAL 6)
#     qt_finalize_executable(xhexviewer)
# endif()
