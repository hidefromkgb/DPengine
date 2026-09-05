cmake_minimum_required(VERSION 3.15)
project(dpe LANGUAGES C CXX)

set(LIB_NAME dpe)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

option(BUILD_SHARED_LIBS OFF) # implicit CMake option!

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(OS_SRC linux)
    set(EXE_NAME linux-gtk)

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

    set(RLS_MAKE_FLAGS -fvisibility=hidden -fexpensive-optimizations
                       -fno-unwind-tables -fno-asynchronous-unwind-tables)
    set(RLS_LINK_FLAGS -s -Wl,--build-id=none)
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(OS_SRC macos)
    set(EXE_NAME macos)

    set(COMMON_INCLUDE_DIRS core)
    set(COMMON_LIBRARY_DIRS)
    set(LIB_LIBRARIES "-framework AppKit" "-framework OpenGL")
    set(EXE_LIBRARIES "-framework AppKit" m)

    set(MAC_MIN "10.7.0")
    set(CMAKE_OSX_DEPLOYMENT_TARGET ${MAC_MIN})
    string(REGEX REPLACE "^([0-9]+)\\..*" "\\1"
           DARWIN_MAJOR_VERSION "${CMAKE_SYSTEM_VERSION}")
    if(DARWIN_MAJOR_VERSION LESS_EQUAL 11)
        add_compile_options(-no-integrated-as -DMAC_OLD)
    endif()
    add_compile_options(-Wall -march=core2 -mstackrealign -fno-stack-protector
                        -fvisibility=hidden -fconstant-cfstrings -fPIC
                        -stdlib=libc++)
    add_link_options(-Wl,-headerpad_max_install_names -stdlib=libc++)
else()
    set(OS_SRC win32)
    message(FATAL_ERROR ${OS_SRC}) # TODO: add win32
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release)
    add_compile_options(-O2 ${RLS_MAKE_FLAGS})
    add_link_options(${RLS_LINK_FLAGS})
else()
    add_compile_options(-g -DDEV_MODE ${DBG_MAKE_FLAGS})
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

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(MAC_APP_MIN_OS_VERSION ${MAC_MIN})
    set(MAC_APP_ICON "main")
    set(MAC_APP_EXE_NAME "DPE")
    set(MAC_APP_BUNDLE_NAME "Desktop Ponies Engine")
    set(MAC_APP_GUI_IDENTIFIER "hidefromkgb.DesktopPoniesEngine")
    set(MAC_APP_CATEGORY_TYPE "public.app-category.entertainment")
    set(MAC_APP_BUNDLE_VERSION "[early alpha]")
    set_target_properties(${EXE_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/${OS_SRC}/rsrc/plist.in
    )
    set(MAIN_ICON ${PROJECT_SOURCE_DIR}/${OS_SRC}/rsrc/${MAC_APP_ICON}.icns)
    target_sources(${EXE_NAME} PRIVATE ${MAIN_ICON})
    set_source_files_properties(${MAIN_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
    add_custom_command(TARGET ${EXE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rename
        "$<TARGET_BUNDLE_DIR:${EXE_NAME}>/Contents/MacOS/${EXE_NAME}"
        "$<TARGET_BUNDLE_DIR:${EXE_NAME}>/Contents/MacOS/${MAC_APP_EXE_NAME}"
    )
endif()
