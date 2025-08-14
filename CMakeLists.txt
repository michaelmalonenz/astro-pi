cmake_minimum_required (VERSION 3.10)
project (astro-pi)
enable_language(CXX)
enable_testing()

set(CMAKE_CXX_STANDARD 23 CACHE INTERNAL "Set C++ standard to C++23")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS}")
set(CMAKE_CXX_FLAGS "-g -ggdb3")


find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBCAMERA REQUIRED IMPORTED_TARGET libcamera)
message(STATUS "libcamera library found:")
message(STATUS "    version: ${LIBCAMERA_VERSION}")
message(STATUS "    libraries: ${LIBCAMERA_LINK_LIBRARIES}")
message(STATUS "    include path: ${LIBCAMERA_INCLUDE_DIRS}")

# libevent is used specifically by simple-cam as its event loop.
# Applications may use a different event handling implementation.
pkg_check_modules(LIBEVENT REQUIRED IMPORTED_TARGET libevent_pthreads)
message(STATUS "libevent_pthreads library found:")
message(STATUS "    version: ${LIBEVENT_VERSION}")
message(STATUS "    libraries: ${LIBEVENT_LINK_LIBRARIES}")
message(STATUS "    include path: ${LIBEVENT_INCLUDE_DIRS}")

find_package(ImageMagick COMPONENTS Magick++)

add_executable(astro-pi
    button.cpp
    camera.cpp
    event_loop.cpp
    image.cpp
    ili9341.cpp
    ssd1351.cpp
    tp28017.cpp
    display.cpp
    astro_camera.cpp
)

add_subdirectory(spidevpp)

target_link_libraries(astro-pi PRIVATE PkgConfig::LIBEVENT PkgConfig::LIBCAMERA spidevpp wiringPi ImageMagick::Magick++)
include_directories(${CMAKE_SOURCE_DIR} ${LIBCAMERA_INCLUDE_DIRS} ${LIBEVENT_INCLUDE_DIRS})
target_include_directories(astro-pi PRIVATE
    "/usr/include/libcamera"
    "stb"
)
