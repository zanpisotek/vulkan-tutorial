cmake_minimum_required(VERSION 3.24)

project(VulkanTutorial LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan 1.4 REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(VulkanTutorial ${SOURCES})

target_compile_features(VulkanTutorial PRIVATE cxx_std_20)

target_include_directories(VulkanTutorial PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/external/stb"
)
target_sources(VulkanTutorial PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/external/stb/stb_image.cpp"
)

set_target_properties(VulkanTutorial PROPERTIES
        CXX_EXTENSIONS OFF
)

target_link_libraries(VulkanTutorial PRIVATE
        Vulkan::Vulkan
        glfw
        glm::glm
        ${CMAKE_DL_LIBS}
)

target_compile_definitions(VulkanTutorial PRIVATE
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

        GLM_FORCE_RADIANS
        GLM_FORCE_DEPTH_ZERO_TO_ONE

        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Release>:RELEASE>
)