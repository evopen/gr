find_package(spirv_cross_core CONFIG REQUIRED)
link_libraries(spirv-cross-core)

find_package(spirv_cross_glsl CONFIG REQUIRED)
link_libraries(spirv-cross-glsl)

find_package(spirv_cross_msl CONFIG REQUIRED)
link_libraries(spirv-cross-msl)

find_package(spirv_cross_reflect CONFIG REQUIRED)
link_libraries(spirv-cross-reflect)

find_package(spirv_cross_util CONFIG REQUIRED)
link_libraries(spirv-cross-util)

find_library(SHADERC_LIBRARY shaderc_combined)
link_libraries(${SHADERC_LIBRARY})

set(MAIN_TARGET ${PROJECT_NAME})
add_executable(${MAIN_TARGET} "gpu-offscreen.cpp" "pch.h" "VulkanBase.cpp")

target_link_libraries(${MAIN_TARGET} PRIVATE ktx)
target_link_libraries(${MAIN_TARGET} PRIVATE base)


#set_target_properties(${MAIN_TARGET} PROPERTIES UNITY_BUILD ON)
target_precompile_headers(${MAIN_TARGET} PRIVATE pch.h)