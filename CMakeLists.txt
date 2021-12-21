cmake_minimum_required(VERSION 3.13)
project(ImageDithering CXX)

set(${PROJECT_NAME}_SOURCES main.cpp
							src/algorithms/dithering.cpp
							src/algorithms/luaalgorithm.cpp
							src/gui/raygui.cpp)

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src/include)

target_link_options(${PROJECT_NAME} PRIVATE -static) # Link staticly
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wno-missing-braces) # Set warning flags

if(CMAKE_BUILD_TYPE MATCHES Debug)
	target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -O3)
	target_link_options(${PROJECT_NAME} PRIVATE -mwindows) # Disable console output
endif()

# raylib
add_subdirectory(external/raylib)
include(cmake/raylibOptions.cmake) # raylib build configuration
target_compile_definitions(raylib PUBLIC SUPPORT_IMAGE_EXPORT) # Force image export support

# Lua
add_subdirectory(external/Lua)

# Tinyfiledialogs
add_subdirectory(external/tinyfiledialogs)

# Links
target_link_libraries(${PROJECT_NAME} raylib)
target_link_libraries(${PROJECT_NAME} lua_static)
target_link_libraries(${PROJECT_NAME} tinyfiledialogs)


# Linking required by raylib
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
