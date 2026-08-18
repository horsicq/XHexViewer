include(../../dep/build_tools/cmake/cpp_standart_setup.cmake)

add_definitions(-DUSE_DEX)
add_definitions(-DUSE_PDF)
add_definitions(-DUSE_ARCHIVE)
add_definitions(-DUSE_YARA)
add_definitions(-DUSE_XSIMD)

include(${CMAKE_CURRENT_LIST_DIR}/../../dep/XHexView/xhexview.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../dep/XAboutWidget/xaboutwidget.cmake)

find_package(Threads REQUIRED)

set(XHEXVIEWER_GUI_TARGET XHexViewer)
set(XHEXVIEWER_GUI_SOURCES
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

if(WIN32 AND EXISTS "${X_WINDOWS_RESOURCE_ICON_RC}")
    list(APPEND XHEXVIEWER_GUI_SOURCES "${X_WINDOWS_RESOURCE_ICON_RC}")
endif()

if(APPLE AND EXISTS "${X_MACOS_ICON}")
    set_source_files_properties("${X_MACOS_ICON}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    list(APPEND XHEXVIEWER_GUI_SOURCES "${X_MACOS_ICON}")
endif()

add_executable(${XHEXVIEWER_GUI_TARGET} WIN32
    ${XHEXVIEWER_GUI_SOURCES}
)

set_target_properties(${XHEXVIEWER_GUI_TARGET} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "${MACOSX_BUNDLE_GUI_IDENTIFIER}"
    OUTPUT_NAME "${X_ORIGINAL_FILENAME}"
)

target_include_directories(${XHEXVIEWER_GUI_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    # XHexView is a large source-only aggregate. MSBuild otherwise submits all
    # sources to one serial cl.exe task, so CMake's --parallel flag cannot help.
    target_compile_options(${XHEXVIEWER_GUI_TARGET} PRIVATE /MP)
endif()

target_link_libraries(${XHEXVIEWER_GUI_TARGET} PRIVATE
    bzip2
    lzma
    zlib
    ppmd
    xarchive_7zip
    capstone
    cppfilt
    yara
    xsimd
    Qt${XHEXVIEWER_QT_MAJOR}::Widgets
    Qt${XHEXVIEWER_QT_MAJOR}::Concurrent
    Qt${XHEXVIEWER_QT_MAJOR}::Network
    Qt${XHEXVIEWER_QT_MAJOR}::PrintSupport
    Qt${XHEXVIEWER_QT_MAJOR}::OpenGL
    Qt${XHEXVIEWER_QT_MAJOR}::Svg
    Qt${XHEXVIEWER_QT_MAJOR}::Sql
    ${XHEXVIEWER_SCRIPT_LIBS}
    Threads::Threads
)

if(WIN32)
    target_link_libraries(${XHEXVIEWER_GUI_TARGET} PRIVATE Wintrust Crypt32)
endif()

if(TARGET xhexviewer_translations)
    add_dependencies(${XHEXVIEWER_GUI_TARGET} xhexviewer_translations)
endif()

if(UNIX AND NOT APPLE)
    set(XHEXVIEWER_RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}")
    set(XHEXVIEWER_DOC_DESTINATION "${CMAKE_INSTALL_DOCDIR}/xhexviewer")
    set(XHEXVIEWER_DATA_DESTINATION "${CMAKE_INSTALL_LIBDIR}/xhexviewer")
    set(XHEXVIEWER_DESKTOP_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/../../res/linux/xhexviewer.desktop.in")
    set(XHEXVIEWER_DESKTOP_FILE "${CMAKE_CURRENT_BINARY_DIR}/xhexviewer.desktop")

    if(EXISTS "${XHEXVIEWER_DESKTOP_TEMPLATE}")
        configure_file("${XHEXVIEWER_DESKTOP_TEMPLATE}" "${XHEXVIEWER_DESKTOP_FILE}" @ONLY)
    endif()
elseif(APPLE)
    set(XHEXVIEWER_RUNTIME_DESTINATION ".")
    set(XHEXVIEWER_DOC_DESTINATION ".")
    set(XHEXVIEWER_DATA_DESTINATION "${X_ORIGINAL_FILENAME}.app/Contents/Resources")
else()
    set(XHEXVIEWER_RUNTIME_DESTINATION ".")
    set(XHEXVIEWER_DOC_DESTINATION ".")
    set(XHEXVIEWER_DATA_DESTINATION ".")
endif()

install(TARGETS ${XHEXVIEWER_GUI_TARGET}
    RUNTIME DESTINATION ${XHEXVIEWER_RUNTIME_DESTINATION}
    BUNDLE DESTINATION .
)

install(DIRECTORY ../../dep/Detect-It-Easy/db DESTINATION "${XHEXVIEWER_DATA_DESTINATION}" OPTIONAL)
install(DIRECTORY ../../dep/Detect-It-Easy/db_custom DESTINATION "${XHEXVIEWER_DATA_DESTINATION}" OPTIONAL)
install(DIRECTORY ../../dep/XInfoDB/info DESTINATION "${XHEXVIEWER_DATA_DESTINATION}" OPTIONAL)
install(DIRECTORY ../../dep/XStyles/qss DESTINATION "${XHEXVIEWER_DATA_DESTINATION}" OPTIONAL)
install(DIRECTORY ../../dep/XYara/yara_rules DESTINATION "${XHEXVIEWER_DATA_DESTINATION}" OPTIONAL)
install(FILES ../../dep/signatures/crypto.db DESTINATION "${XHEXVIEWER_DATA_DESTINATION}/signatures" OPTIONAL)

if(UNIX AND NOT APPLE)
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../README.md"
        DESTINATION ${XHEXVIEWER_DOC_DESTINATION}
        OPTIONAL
    )

    if(EXISTS "${XHEXVIEWER_DESKTOP_FILE}")
        install(FILES "${XHEXVIEWER_DESKTOP_FILE}"
            DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
        install(FILES "${X_RESOURCE_DIR}/main.png"
            DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps"
            RENAME xhexviewer.png
            OPTIONAL)
    endif()
else()
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE"
        DESTINATION .
        RENAME LICENSE.txt
        OPTIONAL
    )

    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/../../README.md"
        "${X_RESOURCE_DIR}/license.txt"
        "${X_RESOURCE_DIR}/readme.txt"
        "${X_WINDOWS_ICON}"
        DESTINATION .
        OPTIONAL
    )

    install(CODE [[file(WRITE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/portable" "")]])
endif()

if(APPLE)
    include(${CMAKE_CURRENT_LIST_DIR}/../../dep/build_tools/cmake/macdeployqt.cmake)
endif()

if(WIN32)
    include(${CMAKE_CURRENT_LIST_DIR}/../../dep/build_tools/cmake/deploy_qt_windows.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/../../dep/build_tools/cmake/deploy_msvc.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/../../dep/build_tools/cmake/deploy_openssl.cmake)
endif()
