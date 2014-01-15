include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(app_SRCS
    unitlauncher.cpp
    sessioninterface.cpp
    sessionmanager.cpp
    main.cpp
)

qt5_add_dbus_adaptor(app_SRCS
    org.lemuri.session.xml
    sessioninterface.h
    SessionInterface
)

qt5_add_dbus_adaptor(app_SRCS
    org.lemuri.session.unit.xml
    unitlauncher.h
    UnitLauncher
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

add_executable(lemuri-session ${app_SRCS})

qt5_use_modules(lemuri-session Core Network DBus Gui)

install(TARGETS lemuri-session DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
