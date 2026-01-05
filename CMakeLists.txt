cmake_minimum_required(VERSION 3.10)
project(WineApplicationManager VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find packages
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Source files for library
set(WRAPPER_SOURCES
    wine_wrapper.cpp
    wine_wrapper_impl.cpp
    wine_executor.cpp
    wine_utils.cpp
    wine_app_manager.cpp
)

# Header files
set(WRAPPER_HEADERS
    wine_wrapper.hpp
)

# Build shared library
add_library(wine_wrapper_shared SHARED ${WRAPPER_SOURCES})
set_target_properties(wine_wrapper_shared PROPERTIES
    OUTPUT_NAME wine_wrapper
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)
target_link_libraries(wine_wrapper_shared Threads::Threads)

# Build static library
add_library(wine_wrapper_static STATIC ${WRAPPER_SOURCES})
set_target_properties(wine_wrapper_static PROPERTIES
    OUTPUT_NAME wine_wrapper
)
target_link_libraries(wine_wrapper_static Threads::Threads)

# Build CLI executable
add_executable(wine-cli wine_cli.cpp)
target_link_libraries(wine-cli wine_wrapper_static Threads::Threads)

# Installation rules
install(TARGETS wine-cli DESTINATION bin)
install(TARGETS wine_wrapper_shared DESTINATION lib)
install(FILES ${WRAPPER_HEADERS} DESTINATION include/wine_wrapper)

# Install Python GUI files
install(FILES 
    wine_gui.py
    wine_gui_main.py
    wine_gui_window.py
    DESTINATION share/wine-app
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

# Create symlink for GUI
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink 
    ${CMAKE_INSTALL_PREFIX}/share/wine-app/wine_gui_window.py 
    ${CMAKE_INSTALL_PREFIX}/bin/wine-gui)")

# CPack configuration for package generation
set(CPACK_PACKAGE_NAME "wine-application-manager")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Wine Application Manager for Linux")
set(CPACK_PACKAGE_VENDOR "Wine App Team")
set(CPACK_PACKAGE_CONTACT "wineapp@example.com")

set(CPACK_GENERATOR "DEB;RPM;TGZ")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "wine, python3-pyqt6")
set(CPACK_RPM_PACKAGE_REQUIRES "wine, python3-qt6")

include(CPack)

# Testing
enable_testing()
add_test(NAME version_test COMMAND wine-cli version)
