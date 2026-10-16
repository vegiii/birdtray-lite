cmake_minimum_required(VERSION 3.16)

project(BirdtrayLite CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 CONFIG REQUIRED COMPONENTS Core Gui Widgets DBus Svg)
find_library(X11_LIBRARY X11 REQUIRED)

option(OPT_THUNDERBIRD_CMDLINE "Default Thunderbird startup command-line" [])
option(OPT_THUNDERBIRD_PROFILE "Default Thunderbird profile path" [])

if(OPT_THUNDERBIRD_CMDLINE)
    message(STATUS "Setting Thunderbird command-line to ${OPT_THUNDERBIRD_CMDLINE}")
    add_definitions(-DOPT_THUNDERBIRD_CMDLINE="${OPT_THUNDERBIRD_CMDLINE}")
endif()

if(OPT_THUNDERBIRD_PROFILE)
    message(STATUS "Setting Thunderbird profile path to ${OPT_THUNDERBIRD_PROFILE}")
    add_definitions(-DOPT_THUNDERBIRD_PROFILE="${OPT_THUNDERBIRD_PROFILE}")
endif()

add_definitions(-DQT_DEPRECATED_WARNINGS)

add_executable(birdtray-lite
        src/main.cpp
        src/birdtrayapp.cpp
        src/colorbutton.cpp
        src/dialogaddeditnewemail.cpp
        src/dialoglogoutput.cpp
        src/dialogsettings.cpp
        src/log.cpp
        src/mailaccountdialog.cpp
        src/modelaccounttree.cpp
        src/modelnewemails.cpp
        src/morkparser.cpp
        src/qtcomponents.cpp
        src/setting_newemail.cpp
        src/settings.cpp
        src/trayicon.cpp
        src/unreadmonitor.cpp
        src/utils.cpp
        src/windowtools.cpp
        src/windowtools_x11.cpp
        src/dialogaddeditnewemail.ui
        src/dialoglogoutput.ui
        src/dialogsettings.ui
        src/mailaccountdialog.ui
        src/resources.qrc)
target_include_directories(birdtray-lite PRIVATE src)
target_link_libraries(birdtray-lite PRIVATE
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::DBus Qt6::Svg ${X11_LIBRARY})

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
