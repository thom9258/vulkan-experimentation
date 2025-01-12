cmake_minimum_required(VERSION 3.2)

project(vulkancpp-test VERSION 0.1.0)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -ggdb")
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} test.cpp)


find_package(SDL2 REQUIRED)
include_directories(${PROJECT_NAME} ${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${SDL2_LIBRARIES})

include(FetchContent)

#-------------------------------------------------------------------------
# Fetch Polymorph
# ------------------------------------------------------------------------
include(FetchContent)
message(STATUS "Fetching polymorph")
FetchContent_Declare(polymorph
    GIT_REPOSITORY git@github.com:thom9258/polymorph.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(polymorph)
target_link_libraries(${PROJECT_NAME} PRIVATE polymorph)

#-------------------------------------------------------------------------
# Fetch Vulkan Setup
# https://stackoverflow.com/questions/69144529/vulkan-hpp-with-cmake
# ------------------------------------------------------------------------
set(VULKAN_VER_MAJOR 1)
set(VULKAN_VER_MINOR 4)
set(VULKAN_VER_PATCH 303)
set(VULKAN_VERSION ${VULKAN_VER_MAJOR}.${VULKAN_VER_MINOR}.${VULKAN_VER_PATCH})

#-------------------------------------------------------------------------
# Fetch Vulkan C Headers
# ------------------------------------------------------------------------
message(STATUS "Fetching Vulkan C Headers")
FetchContent_Declare(
        vulkanheaders
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
        GIT_TAG v${VULKAN_VERSION}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
set(PROJECT_IS_TOP_LEVEL OFF)
FetchContent_MakeAvailable(vulkanheaders) # vulkanheaders has a proper CMakeLists

message(STATUS "Fetching Vulkan C++ Headers")
FetchContent_Declare(
    vulkanhpp
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Hpp.git
    GIT_TAG v${VULKAN_VERSION}
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
# vulkanhpp does not have a proper CMakeLists
FetchContent_GetProperties(vulkanhpp)
FetchContent_Populate(vulkanhpp)
# Define a library for Vulkan
add_library(vulkan INTERFACE)
target_include_directories(vulkan INTERFACE ${glm_INCLUDE_DIRS})

#-------------------------------------------------------------------------
# Fetch glsang
#-------------------------------------------------------------------------
#https://stackoverflow.com/questions/79010013/how-to-fetchcontent-declare-properly-and-also-target-link-these-libraries

#-------------------------------------------------------------------------
# Fetch GLM
#-------------------------------------------------------------------------
message(STATUS "Fetching GLM")
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  #GIT_TAG        bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
  GIT_TAG 	     0.9.9.2
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   FALSE
)
set(GLM_TEST_ENABLE OFF)
FetchContent_MakeAvailable(glm)

#-------------------------------------------------------------------------
# Fetch SDL2
#-------------------------------------------------------------------------
# # https://gist.github.com/Lightnet/398d2ff0b8691aa03520b45b1b4ef69c
# FetchContent_Declare(SDL2
#GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
#GIT_TAG release-2.26.3
#GIT_SHALLOW TRUE
#GIT_PROGRESS FALSE
# )
# FetchContent_MakeAvailable(SDL2)

target_include_directories(${PROJECT_NAME}
  PRIVATE 
  ${glm_SOURCE_DIR}
)

target_link_libraries(
  vulkan
  INTERFACE
  Vulkan::Headers
 
  #SDL2::SDL2-static
)
