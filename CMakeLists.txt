cmake_minimum_required(VERSION 3.5)

project(fat32-emulator LANGUAGES C)

add_executable(fat32-emulator main.c)

include(GNUInstallDirs)
install(TARGETS fat32-emulator
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
