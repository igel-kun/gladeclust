cmake_minimum_required (VERSION 2.6)
project (gcclust)

find_package(GTK)
find_package(PkgConfig)

pkg_check_modules(GTKMM gtkmm-2.4)
pkg_check_modules(GMODULEEXPORT gmodule-export-2.0)
#`pkg-config --cflags gtk+-2.0 gmodule-export-2.0`
#`pkg-config --cflags gtkmm-2.4`

set(DATADIR /share/gcclust/)

add_definitions(-DDATADIR="${CMAKE_INSTALL_PREFIX}${DATADIR}")
add_definitions(-DNODEBUG)

link_directories(
    ${GTKMM_LIBRARY_DIRS}
    ${GMODULEEXPORT_LIBRARY_DIRS}
)
include_directories(
    ${GTKMM_INCLUDE_DIRS}
    ${GMODULEEXPORT_INCLUDE_DIRS}
)

add_library (edit_clusterings src/edit_clusterings.cpp)
add_library (gcclust_window src/gcclust_window.cpp)

add_executable(gcclust src/gladeclust.cpp)
target_link_libraries(gcclust
    ${GTKMM_LIBRARIES}
    ${GMODULEEXPORT_LIBRARIES}
    gcclust_window
    edit_clusterings
)


INSTALL_FILES(${DATADIR} FILES gcclust.ui create_clustering.ui)
INSTALL_PROGRAMS(/bin FILES gcclust)
