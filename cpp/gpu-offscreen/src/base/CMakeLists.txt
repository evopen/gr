#file(GLOB BASE_SRC "*.cpp" "*.hpp")
#file(GLOB BASE_HEADERS "*.hpp")

set(BASE_SRC "VulkanTools.cpp" "VulkanTexture.hpp")


add_library(base ${BASE_SRC})
target_link_libraries(base ${Vulkan_LIBRARY} ${ASSIMP_LIBRARIES} ${WINLIBS} ktx)

