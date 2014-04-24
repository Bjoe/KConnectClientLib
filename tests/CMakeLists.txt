
# cmake example from http://qt-project.org/doc/qt-5.1/qtdoc/cmake-manual.html
find_package(Qt5Test REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)

set(RESOURCES ../resources.qrc)

set(QML_PATH "qrc:/assets/qml/main.qml")

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/testconfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/testconfig.h"
  )

include_directories(
  "../src" 
  "${CMAKE_CURRENT_BINARY_DIR}/../src"
)

set(test_additional_lib
    kconnectcl
    Qt5::Core
    Qt5::Qml
    Qt5::Quick
    Qt5::Widgets
    ${QCA2_LIBRARIES}
)

#set(test_additional
#    foo.cpp
#)

set(CMAKE_AUTOMOC OFF)

qt_test(foo exampletest.cpp)

set(CMAKE_AUTOMOC ON)

qt5_add_resources(QRC ${RESOURCES})
add_executable(kconnTestCl main.cpp gui/mainwindowuicontroller.cpp gui/kconnecttestclient.cpp ${QRC})
target_link_libraries(kconnTestCl kconnectcl Qt5::Core Qt5::Widgets Qt5::Qml Qt5::Quick ${QCA2_LIBRARIES})
