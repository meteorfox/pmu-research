cmake_minimum_required(VERSION 3.13.0)
project(
  pmu-research
  VERSION 0.1.0
  LANGUAGES C CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_CXX_CLANG_TIDY clang-tidy-9 "--format-style=file")

# find_package(Boost REQUIRED COMPONENTS program_options)

add_library(project_warnings INTERFACE)

if(CMAKE_COMPILER_IS_GNUCC)
  option(ENABLE_COVERAGE "Enable coverage reporting for gcc/clang" FALSE)

  if(ENABLE_COVERAGE)
    add_compile_options(--coverage -O0)
  endif()
endif()

target_compile_options(
  project_warnings
  INTERFACE -Wall
            -Wextra # reasonable and standard
            -Wshadow # warn the user if a variable declaration shadows one from
                     # a parent context
            -Wnon-virtual-dtor # warn the user if a class with virtual functions
                               # has a non-virtual destructor. This helps catch
                               # hard to track down memory errors
            -Wold-style-cast # warn for c-style casts
            -Wcast-align # warn for potential performance problem casts
            -Wunused # warn on anything being unused
            -Woverloaded-virtual # warn if you overload (not override) a virtual
                                 # function
            -Wpedantic # warn if non-standard C++ is used
            -Wconversion # warn on type conversions that may lose data
            -Wsign-conversion # warn on sign conversions
            -Wmisleading-indentation # warn if identation implies blocks where
                                     # blocks do not exist
            -Wduplicated-cond # warn if if / else chain has duplicated
                              # conditions
            -Wduplicated-branches # warn if if / else branches have duplicated
                                  # code
            -Wlogical-op # warn about logical operations being used where
                         # bitwise were probably wanted
            -Wnull-dereference # warn if a null dereference is detected
            -Wuseless-cast # warn if you perform a cast to the same type
            -Wdouble-promotion # warn if float is implicit promoted to double
            -Wformat=2 # warn on security issues around functions that format
                       # output (ie printf)
)

# set_property(TARGET tgt PROPERTY C_STANDARD 11)

include(ExternalProject)
ExternalProject_Add(
  libpfc
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libpfc
  DOWNLOAD_COMMAND ""
  INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/libpfc-build
  CONFIGURE_COMMAND meson <SOURCE_DIR> -Dbuildtype=release
                    --prefix=<INSTALL_DIR>
  BUILD_COMMAND ninja
  INSTALL_COMMAND ninja install
  BUILD_BYPRODUCTS <INSTALL_DIR>/lib/x86_64-linux-gnu/libpfc.so)

ExternalProject_Get_Property(libpfc SOURCE_DIR)
ExternalProject_Get_Property(libpfc INSTALL_DIR)

add_library(pfc::libpfc STATIC IMPORTED)
set_target_properties(
  pfc::libpfc PROPERTIES IMPORTED_LOCATION
                         ${INSTALL_DIR}/lib/x86_64-linux-gnu/libpfc.so)

add_executable(pfc main.c)

add_dependencies(pfc::libpfc libpfc)
target_include_directories(pfc::libpfc INTERFACE ${SOURCE_DIR}/include)
target_link_libraries(pfc::libpfc
                      INTERFACE ${INSTALL_DIR}/lib/x86_64-linux-gnu/libpfc.so)

target_link_libraries(pfc pfc::libpfc)
