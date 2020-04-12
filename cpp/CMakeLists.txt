# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.16)

project ("cpp" CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

find_package(glm CONFIG REQUIRED)
link_libraries(glm)

find_package(OpenCV CONFIG REQUIRED)
link_libraries(${OpenCV_LIBS})

find_package(GSL REQUIRED)
link_libraries(GSL::gsl GSL::gslcblas)

find_package(unofficial-cairo CONFIG REQUIRED)
link_libraries(unofficial::cairo::cairo unofficial::cairo::cairo-gobject)

set(LLVM_DIRECTORY "C:/Users/DHH/scoop/apps/llvm/10.0.0")

set(OpenMP_LIBRARY_DIR "${LLVM_DIRECTORY}/lib")
set(OpenMP_CXX_FLAGS "-Xclang -fopenmp")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

# Tell CMake where to find the OpenMP libraries.
link_directories(${OpenMP_LIBRARY_DIR})
link_libraries(libomp libiomp5md)



find_library(AVDEVICE_LIBRARY avdevice)
find_library(AVFILTER_LIBRARY avfilter)
find_library(SWSCALE_LIBRARY swscale)
find_library(AVFORMAT_LIBRARY avformat)
find_library(AVCODEC_LIBRARY avcodec)
find_library(SWRESAMPLE_LIBRARY swresample)
find_library(AVRESAMPLE_LIBRARY avresample)
find_library(AVUTIL_LIBRARY avutil)
link_libraries(${AVDEVICE_LIBRARY} 
    ${AVFILTER_LIBRARY}
    ${SWSCALE_LIBRARY} 
    ${AVFORMAT_LIBRARY} 
    ${AVCODEC_LIBRARY} 
    ${SWRESAMPLE_LIBRARY}
    ${AVRESAMPLE_LIBRARY} 
    ${AVUTIL_LIBRARY}
)

# Include sub-projects.
add_subdirectory ("offline")
add_subdirectory ("gpu-offscreen")

#include_directories(${VCPKG_INCLUDE_DIR})
#message(${VCPKG_INCLUDE_DIR})

