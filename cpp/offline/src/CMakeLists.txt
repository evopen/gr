set(MAIN_TARGET ${PROJECT_NAME})
add_executable(${MAIN_TARGET} "offline.cpp" "writer.cpp" "library.h")

#set_target_properties(${MAIN_TARGET} PROPERTIES UNITY_BUILD ON)
target_precompile_headers(${MAIN_TARGET} PRIVATE pch.h)


add_executable(playground "playground.cpp" "writer.cpp")
#target_precompile_headers(playground PRIVATE pch.h)


add_custom_command(
    TARGET playground 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${LLVM_DIRECTORY}/bin/libomp.dll
        ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_command(
    TARGET ${MAIN_TARGET} 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${LLVM_DIRECTORY}/bin/libomp.dll
        ${CMAKE_CURRENT_BINARY_DIR}
)