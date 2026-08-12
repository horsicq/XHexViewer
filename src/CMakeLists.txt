if(TARGET Qt6::Core OR TARGET Qt6::Widgets)
    set(XHEXVIEWER_QT_MAJOR 6)
elseif(TARGET Qt5::Core OR TARGET Qt5::Widgets)
    set(XHEXVIEWER_QT_MAJOR 5)
else()
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
    set(XHEXVIEWER_QT_MAJOR ${QT_VERSION_MAJOR})
endif()

set(XHEXVIEWER_QT_COMPONENTS
    Widgets
    Concurrent
    Network
    PrintSupport
    OpenGL
    Svg
    Sql
    LinguistTools
)

if(XHEXVIEWER_QT_MAJOR EQUAL 5)
    find_package(Qt5 REQUIRED COMPONENTS ${XHEXVIEWER_QT_COMPONENTS} Script ScriptTools)
    set(XHEXVIEWER_SCRIPT_LIBS Qt5::Script Qt5::ScriptTools)
elseif(XHEXVIEWER_QT_MAJOR GREATER_EQUAL 6)
    find_package(Qt${XHEXVIEWER_QT_MAJOR} REQUIRED COMPONENTS ${XHEXVIEWER_QT_COMPONENTS} Qml)
    set(XHEXVIEWER_SCRIPT_LIBS Qt${XHEXVIEWER_QT_MAJOR}::Qml)

    # A few shared, source-only widgets still refer to this compatibility
    # target in their CMake fragments.
    if(NOT TARGET Qt5::Widgets)
        add_library(Qt5::Widgets INTERFACE IMPORTED)
        set_property(TARGET Qt5::Widgets PROPERTY
            INTERFACE_LINK_LIBRARIES Qt${XHEXVIEWER_QT_MAJOR}::Widgets
        )
    endif()
endif()

set(QT_VERSION_MAJOR ${XHEXVIEWER_QT_MAJOR})

file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../release_version.txt" X_PROJECT_VERSION)
project(XHexViewer VERSION ${X_PROJECT_VERSION} LANGUAGES CXX)

set(X_COMPANYNAME "ntinfo")
set(X_PROJECTNAME "XHexViewer")
set(X_MAINTAINER "horsicq@gmail.com")
set(X_DESCRIPTION "XHexViewer is a HEX viewer")
set(X_HOMEPAGE "http://ntinfo.biz")
set(X_ORIGINAL_FILENAME "xhexviewer")

if(NOT DEFINED MACOSX_BUNDLE_GUI_IDENTIFIER OR "${MACOSX_BUNDLE_GUI_IDENTIFIER}" STREQUAL "")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.xhexviewer")
endif()
set(BUNDLE_ID_OPTION "${MACOSX_BUNDLE_GUI_IDENTIFIER}")

set(X_RESOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../res")
set(X_WINDOWS_ICON "${X_RESOURCE_DIR}/main.ico")
set(X_WINDOWS_RESOURCE_RC_IN "${X_RESOURCE_DIR}/resource.rc.in")
set(X_WINDOWS_RESOURCE_ICON_RC_IN "${X_RESOURCE_DIR}/resource_icon.rc.in")
set(X_WINDOWS_RESOURCE_RC "${X_RESOURCE_DIR}/resource.rc")
set(X_WINDOWS_RESOURCE_ICON_RC "${X_RESOURCE_DIR}/resource_icon.rc")
set(X_MACOS_ICON "${X_RESOURCE_DIR}/main.icns")
set(X_MACOS_INFO_PLIST_IN "${X_RESOURCE_DIR}/Info.plist.in")
set(X_MACOS_INFO_PLIST "${X_RESOURCE_DIR}/Info.plist")

set(XHEXVIEWER_DEPLOY_INIT "${CMAKE_CURRENT_LIST_DIR}/../dep/build_tools/cmake/deploy_init.cmake")
if(EXISTS "${XHEXVIEWER_DEPLOY_INIT}"
   AND EXISTS "${X_WINDOWS_RESOURCE_RC_IN}"
   AND EXISTS "${X_WINDOWS_RESOURCE_ICON_RC_IN}"
   AND EXISTS "${X_MACOS_ICON}"
   AND EXISTS "${X_MACOS_INFO_PLIST_IN}")
    include("${XHEXVIEWER_DEPLOY_INIT}")
else()
    message(STATUS "Skipping deploy_init.cmake because packaging resource templates are missing")
endif()

set(XHEXVIEWER_TRANSLATION_HELPER "${CMAKE_CURRENT_LIST_DIR}/../dep/build_tools/cmake/deploy_translation.cmake")
if(EXISTS "${XHEXVIEWER_TRANSLATION_HELPER}")
    include("${XHEXVIEWER_TRANSLATION_HELPER}")

    set(XHEXVIEWER_TS_FILES
        "${X_RESOURCE_DIR}/translation/xhexviewer_ar.ts"
        "${X_RESOURCE_DIR}/translation/xhexviewer_bn.ts"
        "${X_RESOURCE_DIR}/translation/xhexviewer_de.ts"
    )

    deploy_add_translations(
        TARGET_NAME xhexviewer_translations
        ADD_TO_ALL
        SOURCE_DIRS "${CMAKE_CURRENT_LIST_DIR}"
        TS_FILES ${XHEXVIEWER_TS_FILES}
        INSTALL_DESTINATION lang
        WINDOWS_APPDATA_SUBDIR "${X_PROJECTNAME}"
    )
endif()

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../dep/XCppfilt" XCppfilt)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../dep/XCapstone" XCapstone)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../dep/XArchive" XArchive)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../dep/XYara" XYara)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../dep/Formats/xsimd" XSIMD)
add_subdirectory(gui)
