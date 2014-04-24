cmake_minimum_required(VERSION 2.8.11)

project(KConnectClientLib)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" CACHE PATH "cmake modul path") 
include(BuildUtil)

# The version number.
set (Project_VERSION_MAJOR 0)
set (Project_VERSION_MINOR 1)
set (Project_VERSION_PATCH 0)

# activate C++ 11
add_definitions(-std=c++11)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# QML debuggen
#add_definitions(-DQT_QML_DEBUG)
# and set environment
# QML_COMPILER_DUMP=1
# QML_DISABLE_OPTIMIZER=1
# argument:
# $ executable -qmljsdebugger=port:3768,block

#SET(ENV{PKG_CONFIG_PATH} "foo/bar/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
# find_package(PkgConfig)
# pkg_check_modules(FOO foo)
# include_directories(${FOO_INCLUDE_DIRS})
# link_directories(${FOO_LIBRARY_DIRS})
# target_link_libraries(${FOO_LDFLAGS})

# cmake example from http://qt-project.org/doc/qt-5.0/qtdoc/cmake-manual.html
# Find the QtWidgets library
find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)

#find_package(QJSON 0.8.1 REQUIRED)
find_package(QCA2 REQUIRED)

include_directories(
    ${QCA2_INCLUDE_DIRS}
)

if(${DOC})
    find_doxygen()
endif(${DOC})

add_subdirectory(src)

if(${TEST})
    # The following are required to uses CTest, Dart and the Cdash dashboard
    enable_testing(true)
    include(CTest)
    add_subdirectory(tests)
endif(${TEST})
