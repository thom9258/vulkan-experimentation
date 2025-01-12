cmake_minimum_required(VERSION 3.2)

project(vulkan-tutorial VERSION 0.1.0)

include(FetchContent)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -ggdb")
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} main.cpp)

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

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

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
# Include and Link Everything
#-------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME}
  PRIVATE 
    ${glm_INCLUDE_DIRS}
    ${glm_SOURCE_DIR}
    ${Vulkan_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} 
  PRIVATE
	polymorph::polymorph
    ${Vulkan_LIBRARIES}
	${SDL2_LIBRARIES}
)
