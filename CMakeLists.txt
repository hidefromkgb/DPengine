cmake_minimum_required(VERSION 3.15)
project(dpe LANGUAGES C CXX)

set(LIB_NAME dpe)
set(EXE_NAME linux-gtk)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

option(BUILD_SHARED_LIBS OFF) # implicit CMake option!

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(OS_SRC linux)

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK REQUIRED gtk+-2.0)
    pkg_check_modules(GTKGLEXT REQUIRED gtkglext-1.0 gtkglext-x11-1.0)
    add_compile_definitions(GTK_DISABLE_DEPRECATED)

    set(COMMON_INCLUDE_DIRS ${GTK_INCLUDE_DIRS} ${GTKGLEXT_INCLUDE_DIRS} core)
    set(COMMON_LIBRARY_DIRS ${GTK_LIBRARY_DIRS} ${GTKGLEXT_LIBRARY_DIRS})
    set(LIB_LIBRARIES ${GTK_LIBRARIES} ${GTKGLEXT_LIBRARIES} Xrender X11 GL)
    set(EXE_LIBRARIES ${GTK_LIBRARIES} ${GTKGLEXT_LIBRARIES} m curl ssl crypto)

    add_compile_options(-ffunction-sections -fdata-sections -fno-stack-protector
			-pthread -Wall)
    add_link_options(-Wl,--gc-sections)

    set(RLS_MAKE_FLAGS -O2 -fexpensive-optimizations -fvisibility=hidden
		       -fno-unwind-tables -fno-asynchronous-unwind-tables)
    set(RLS_LINK_FLAGS -s -Wl,--build-id=none)
    set(DBG_MAKE_FLAGS -g)
    set(DBG_LINK_FLAGS)
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(OS_SRC macos)
    message(FATAL_ERROR ${OS_SRC}) # TODO: add macos
else()
    set(OS_SRC win32)
    message(FATAL_ERROR ${OS_SRC}) # TODO: add win32
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release)
    add_compile_options(${RLS_MAKE_FLAGS})
    add_link_options(${RLS_LINK_FLAGS})
else()
    add_compile_options(${DBG_MAKE_FLAGS})
    add_link_options(${DBG_LINK_FLAGS})
endif()

include_directories(core ${COMMON_INCLUDE_DIRS})
link_directories(${COMMON_LIBRARY_DIRS})

file(GLOB_RECURSE LIB_SRC CONFIGURE_DEPENDS core/*.c*)
add_library(${LIB_NAME} ${LIB_SRC} ${OS_SRC}/lib.c)
target_link_libraries(${LIB_NAME} PRIVATE ${LIB_LIBRARIES})

file(GLOB_RECURSE EXE_SRC CONFIGURE_DEPENDS exec/*.c*)
add_executable(${EXE_NAME} ${EXE_SRC} ${OS_SRC}/run.c)
target_link_libraries(${EXE_NAME} PRIVATE ${LIB_NAME} ${EXE_LIBRARIES})
