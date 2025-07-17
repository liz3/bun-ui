project(gtk-saver)
cmake_minimum_required(VERSION 3.6)

add_executable(gtk-saver main.c)


find_package (PkgConfig REQUIRED)
pkg_check_modules (GTK3 REQUIRED gtk+-3.0)

target_include_directories(gtk-saver PUBLIC ${GTK3_INCLUDE_DIRS})
target_link_libraries(gtk-saver PUBLIC ${GTK3_LIBRARIES})