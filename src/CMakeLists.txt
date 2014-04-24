
set(QT_SRCS

    daemon.cpp
    device.cpp
    networkpackage.cpp
    filetransferjob.cpp

    #plugins/pluginloader.h


    backends/linkprovider.cpp
    backends/devicelink.cpp
    backends/lan/landevicelink.cpp
    backends/lan/lanlinkprovider.cpp
    backends/lan/uploadjob.cpp
    backends/lan/downloadjob.cpp
    backends/lan/socketlinereader.cpp
)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
  )

add_library(kconnectcl STATIC ${QT_SRCS})
target_link_libraries(kconnectcl Qt5::Network Qt5::Widgets Qt5::Qml ${QCA2_LIBRARIES})
