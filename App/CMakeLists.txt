cmake_minimum_required(VERSION 3.16)

project(XYModem VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS SerialPort)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

set(OPENSSL_ROOT_DIR "D:/OpenSSL-MinGW64")
find_package(OpenSSL REQUIRED)

include_directories(inc)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)
set(PROJECT_SOURCES
    inc/widget.h
    inc/serial.h
    inc/xymodem.h
    inc/textbrowser.h
    inc/plaintextedit.h
    inc/encryptdialog.h
    src/main.cpp
    src/widget.cpp
    src/serial.cpp
    src/xymodem.cpp
    src/encryptdialog.cpp
    ui/widget.ui
    ui/encryptdialog.ui
    resource/resource.qrc
    resource/appIcon.rc
)
set(TS_FILES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resource/translations/")
set(TS_FILES ${TS_FILES_PATH}en_US.ts)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(XYModem
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
    qt_add_lupdate(XYModem
        TS_FILES ${TS_FILES}
        OPTIONS -noobsolete
    )

# Define target properties for Android with Qt 6 as:
# set_property(TARGET XYModem APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
# ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(XYModem SHARED
            ${PROJECT_SOURCES}
        )

    # Define properties for Android with Qt 5 after find_package() calls as:
    # set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(XYModem
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(XYModem PRIVATE Qt6::Core)
target_link_libraries(XYModem PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(XYModem PRIVATE Qt6::SerialPort)
target_link_libraries(XYModem PRIVATE OpenSSL::Crypto OpenSSL::SSL)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.XYModem)
endif()

set_target_properties(XYModem PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS XYModem
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(XYModem)
endif()
