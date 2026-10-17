cmake_minimum_required(VERSION 3.16)

project(BirdtrayLite CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 CONFIG REQUIRED COMPONENTS Core Gui Widgets DBus)

add_executable(birdtray-lite
        src/birdtrayapp.cpp
        src/dialogsettings.cpp
        src/foldertree.cpp
        src/log.cpp
        src/morkparser.cpp
        src/settings.cpp
        src/thunderbird.cpp
        src/trayicon.cpp
        src/unreadmonitor.cpp
        src/windowtools.cpp
        src/dialogsettings.ui
        src/resources.qrc)
target_include_directories(birdtray-lite PRIVATE src)
target_link_libraries(birdtray-lite PRIVATE
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::DBus)

include(GNUInstallDirs)
set(APP_ID io.github.vegiii.BirdtrayLite)
install(TARGETS birdtray-lite RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES src/res/${APP_ID}.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES src/res/${APP_ID}.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo)
foreach(size 32 48 64 128)
    install(FILES src/res/icons/${size}/${APP_ID}.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${size}x${size}/apps)
endforeach()
install(FILES src/res/birdtray.svg
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
        RENAME ${APP_ID}.svg)
