cmake_minimum_required(VERSION 2.8.12)
project(LED_encoding)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
###
# Configuration Options
###

###
# Dependencies
###

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(SOURCES
    LED_encoding.cpp
)

add_executable(LED_encoding ${SOURCES})

install(TARGETS LED_encoding
    RUNTIME DESTINATION bin)

set(APPS
    "\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/LED_encoding${CMAKE_EXECUTABLE_SUFFIX}")
install(CODE "include(BundleUtilities)
    fixup_bundle(\"${APPS}\" \"\" \"${libSearchDirs}\")")
