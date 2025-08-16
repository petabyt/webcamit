cmake_minimum_required(VERSION 3.14)
project(webcamit)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
include(ExternalProject)

set(CMAKE_C_FLAGS_RELEASE -Os)
set(CMAKE_CXX_FLAGS_RELEASE -Os)
set(CMAKE_C_FLAGS_DEBUG -g)
set(CMAKE_CXX_FLAGS_DEBUG -g)
set(CMAKE_C_FLAGS_RELWITHDEBUG -g)
set(CMAKE_CXX_FLAGS_RELWITHDEBUG -g)

find_package(PkgConfig REQUIRED)
pkg_check_modules(UDEV IMPORTED_TARGET GLOBAL libudev)

# libpict
option(PTP_DEFAULT_LOGGING "Include default log/panic/error implementation" ON)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libpict ${CMAKE_CURRENT_BINARY_DIR}/libpict)

# libjpeg-turbo
ExternalProject_Add(libjpeg_turbo_ext
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/libjpeg-turbo
    USES_TERMINAL_CONFIGURE TRUE
    USES_TERMINAL_BUILD TRUE
    CMAKE_ARGS
        -DWITH_JPEG8=TRUE
        -DWITH_TURBOJPEG=TRUE
        -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/libjpeg
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        -DENABLE_SHARED=OFF
        -DENABLE_STATIC=ON
    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/libjpeg/lib/libjpeg.a;${CMAKE_CURRENT_BINARY_DIR}/libjpeg/lib/libturbojpeg.a
)
add_library(libjpeg-turbo SHARED IMPORTED)
add_dependencies(libjpeg-turbo libjpeg_turbo_ext)
set_target_properties(libjpeg-turbo PROPERTIES
	IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/libjpeg/lib/libjpeg.a
	INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/libs/libjpeg-turbo/src
)
add_library(libturbojpeg-turbo SHARED IMPORTED)
set_target_properties(libturbojpeg-turbo PROPERTIES
	IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/libjpeg/lib/libturbojpeg.a
	INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/libs/libjpeg-turbo/src
)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/libjpeg-turbo2 ${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo)

# libgphoto2
set(LIBGPHOTO2_SRC_DIR ${CMAKE_SOURCE_DIR}/libs/libgphoto2)
set(LIBGPHOTO2_BUILD_DIR ${CMAKE_BINARY_DIR}/libgphoto2-build)
set(LIBGPHOTO2_INSTALL_DIR ${CMAKE_BINARY_DIR}/libgphoto2-install)
ExternalProject_Add(libgphoto2_ext
    USES_TERMINAL_CONFIGURE TRUE
    USES_TERMINAL_BUILD TRUE
	PREFIX ${CMAKE_BINARY_DIR}/libgphoto2_ext
	SOURCE_DIR ${LIBGPHOTO2_SRC_DIR}
	BINARY_DIR ${LIBGPHOTO2_BUILD_DIR}
	CONFIGURE_COMMAND meson setup --reconfigure --libdir lib ${LIBGPHOTO2_BUILD_DIR} ${LIBGPHOTO2_SRC_DIR} --prefix ${LIBGPHOTO2_INSTALL_DIR} "-Dc_args=${CMAKE_C_FLAGS}" "-Diolibs=disk,libusb1,usb"
	BUILD_COMMAND ninja -C ${LIBGPHOTO2_BUILD_DIR}
	#UPDATE_COMMAND ninja -C ${LIBGPHOTO2_BUILD_DIR} install
	BUILD_BYPRODUCTS ${LIBGPHOTO2_INSTALL_DIR}/lib/libgphoto2.so;${LIBGPHOTO2_INSTALL_DIR}/lib/libgphoto2_port.so
	INSTALL_COMMAND ninja -C ${LIBGPHOTO2_BUILD_DIR} install
)
add_library(libgphoto2 SHARED IMPORTED)
add_dependencies(libgphoto2 libgphoto2_ext)
set_target_properties(libgphoto2 PROPERTIES
	IMPORTED_LOCATION ${LIBGPHOTO2_INSTALL_DIR}/lib/libgphoto2.so
	INTERFACE_INCLUDE_DIRECTORIES "${LIBGPHOTO2_SRC_DIR};${LIBGPHOTO2_SRC_DIR}/libgphoto2_port"
)
add_library(libgphoto2_port SHARED IMPORTED)
set_target_properties(libgphoto2_port PROPERTIES
    IMPORTED_LOCATION ${LIBGPHOTO2_INSTALL_DIR}/lib/libgphoto2_port.so
    INTERFACE_INCLUDE_DIRECTORIES "${LIBGPHOTO2_SRC_DIR}/libgphoto2_port"
)

add_executable(webcamit
    src/main.c
    src/usb.c
)
target_link_libraries(webcamit PkgConfig::UDEV libpict libgphoto2 libgphoto2_port libjpeg-turbo libturbojpeg-turbo)
target_include_directories(webcamit PRIVATE src/)
target_compile_options(webcamit PRIVATE -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types)

install(
    TARGETS webcamit
)
