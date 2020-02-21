cmake_minimum_required(VERSION 3.13.0)
project(
  third_party
  VERSION 0.0.0
  LANGUAGES C CXX ASM)

find_program(MAKE_EXE NAMES gmake nmake make)

add_custom_target(pfckmod
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/libpfc/kmod/mesonbuildkmod.sh ${CMAKE_CURRENT_SOURCE_DIR}/libpfc ${CMAKE_CURRENT_BINARY_DIR}
)


add_library(pfc ${CMAKE_CURRENT_SOURCE_DIR}/libpfc/src/libpfc.c)
target_include_directories(pfc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libpfc/include>
    $<INSTALL_INTERFACE:libpfc/include>
)
target_link_libraries(pfc m)
set_target_properties(pfc PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS ON
)
add_dependencies(pfc pfckmod)

add_executable(pfcutil
    ${CMAKE_CURRENT_SOURCE_DIR}/libpfc/src/pfcutil.c
)
target_link_libraries(pfcutil pfc)


add_executable(pfcdemo
    ${CMAKE_CURRENT_SOURCE_DIR}/libpfc/src/pfcdemo.c
)
target_link_libraries(pfcdemo pfc)


add_executable(pfcdemoreftsc
    ${CMAKE_CURRENT_SOURCE_DIR}/libpfc/src/pfcdemoreftsc.cpp
)
target_link_libraries(pfcdemoreftsc pfc)
set_target_properties(pfcdemoreftsc PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

add_executable(pfctbhit
    ${CMAKE_CURRENT_SOURCE_DIR}/libpfc/src/pfctbhit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libpfc/src/pfctbhitasm.S
)
target_link_libraries(pfctbhit pfc)
