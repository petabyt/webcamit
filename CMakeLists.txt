cmake_minimum_required(VERSION 3.14)
project(webcamit)

option(WEBCAMIT_GPHOTO_SUBMODULE "" ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(UDEV IMPORTED_TARGET GLOBAL libudev)

option(PTP_DEFAULT_LOGGING "Include default log/panic/error implementation" OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libpict ${CMAKE_CURRENT_BINARY_DIR}/libpict)

if(WEBCAMIT_GPHOTO_SUBMODULE)
    include(ExternalProject)

    set(LIBGPHOTO2_SRC_DIR ${CMAKE_SOURCE_DIR}/libs/libgphoto2)
    set(LIBGPHOTO2_BUILD_DIR ${CMAKE_BINARY_DIR}/libgphoto2-build)

    ExternalProject_Add(libgphoto2_ext
    	PREFIX ${CMAKE_BINARY_DIR}/libgphoto2_ext
    	SOURCE_DIR ${LIBGPHOTO2_SRC_DIR}
    	BINARY_DIR ${LIBGPHOTO2_BUILD_DIR}
    	CONFIGURE_COMMAND meson setup ${LIBGPHOTO2_BUILD_DIR} ${LIBGPHOTO2_SRC_DIR}
    	BUILD_COMMAND ninja -C ${LIBGPHOTO2_BUILD_DIR}
    	BUILD_BYPRODUCTS ${LIBGPHOTO2_BUILD_DIR}/libgphoto2/libgphoto2.so
    	INSTALL_COMMAND ""
    )

    add_library(libgphoto2 SHARED IMPORTED)
    add_dependencies(libgphoto2 libgphoto2_ext)

    set_target_properties(libgphoto2 PROPERTIES
    	IMPORTED_LOCATION ${LIBGPHOTO2_BUILD_DIR}/libgphoto2/libgphoto2.so
    	INTERFACE_INCLUDE_DIRECTORIES ${LIBGPHOTO2_BUILD_DIR}
    )
endif()

add_executable(webcamit
    src/main.c
    src/usb.c
)
target_link_libraries(webcamit PkgConfig::UDEV libpict libgphoto2)
target_include_directories(webcamit PRIVATE src/)
target_compile_options(webcamit PRIVATE -Wall -fPIC -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types)

install(
    TARGETS webcamit
)
