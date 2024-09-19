cmake_minimum_required(VERSION 3.29)

# Set Vcpkg toolchain file
include(C:/vcpkg/scripts/buildsystems/vcpkg.cmake)

# Project Name
project(HackArena2024H2_Cxx)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED COMPONENTS system beast asio)
find_package(nlohmann_json REQUIRED)

# Include Boost directories
include_directories(${Boost_INCLUDE_DIRS})

# Add executable
add_executable(HackArena2024H2_Cxx src/main.cpp
        src/web-socket-client.cpp
        src/web-socket-client.h
        src/packet.h
        src/pch.h
        src/agent/agent.cpp
        src/agent/agent.h
        src/processed-packets.h
        src/handler.cpp
        src/handler.h)

target_precompile_headers(HackArena2024H2_Cxx PRIVATE src/pch.h)

# Define preprocessor macros
if(WIN32)
    target_compile_definitions(HackArena2024H2_Cxx PRIVATE _WIN32_WINDOWS=0x0A00)
elseif(UNIX)
    # Unix-specific settings
endif()

# Link Boost libraries
target_link_libraries(HackArena2024H2_Cxx PRIVATE Boost::system nlohmann_json::nlohmann_json)

# Ensure static linking
set(BOOST_USE_STATIC_LIBS ON)
set(BOOST_USE_STATIC_RUNTIME ON)