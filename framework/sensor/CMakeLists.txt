cmake_minimum_required(VERSION 2.8)
PROJECT(sensor_manager)

include(FindPkgConfig)

#dependencies
pkg_check_modules(GLIB2	glib-2.0)
pkg_check_modules(DBUS1	dbus-1)
pkg_check_modules(DBUSGLIB1 dbus-glib-1)

FIND_PACKAGE(PkgConfig)
add_definitions("`pkg-config --libs --cflags dbus-1 glib-2.0 dbus-glib-1`")

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE RELEASE)

set(SM_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SM_sources ${SM_SOURCE_DIR}/sensor_manager.c ${SM_SOURCE_DIR}/request.c ${SM_SOURCE_DIR}/devices.c ${SM_SOURCE_DIR}/cJSON.c)

ADD_EXECUTABLE(ant-sensor ${SM_sources})
set(COMPILE_FLAGS -Wall -O2 -fPIC )

TARGET_COMPILE_OPTIONS(ant-sensor PRIVATE ${COMPILE_FLAGS})
TARGET_LINK_LIBRARIES(ant-sensor pthread dl c m dbus-glib-1 dbus-1 glib-2.0)

set_target_properties(ant-sensor
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/bin)

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/inc
	/usr/local/include
  /usr/local/lib
	/usr/include
)

LINK_DIRECTORIES(
	/usr/local/lib/pkgconfig
  /usr/lib
	/usr/local/lib
	/usr/lib/dbus-1.0
)

