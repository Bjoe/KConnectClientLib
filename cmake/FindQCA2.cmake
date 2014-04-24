# - Try to find QCA2
# Once done, this will define
#
#  QCA2_FOUND - system has QCA2
#  QCA2_INCLUDE_DIRS - the QCA2 include directories
#  QCA2_LIBRARIES - link these to use QCA2

if (NOT QCA2_FOUND) 
  include(LibFindMacros)

# Dependencies
#libfind_package(QCA2 qca2)

# Use pkg-config to get hints about paths
  libfind_pkg_check_modules(QCA2_PKGCONF qca2)

# Include dir
  find_path(QCA2_INCLUDE_DIR
    NAMES QtCrypto
    PATHS ${QCA2_PKGCONF_INCLUDE_DIRS}
  )

# Finally the library itself
  find_library(QCA2_LIBRARY
    NAMES qca
    PATHS ${QCA2_PKGCONF_LIBRARY_DIRS}
  )

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
  set(QCA2_PROCESS_INCLUDES QCA2_INCLUDE_DIR QCA2_INCLUDE_DIRS)
  set(QCA2_PROCESS_LIBS QCA2_LIBRARY QCA2_LIBRARIES)
  libfind_process(QCA2)
endif(NOT QCA2_FOUND)