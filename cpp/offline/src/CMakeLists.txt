set(MAIN_TARGET ${PROJECT_NAME})
add_executable(${MAIN_TARGET} "offline.cpp")

#set_target_properties(${MAIN_TARGET} PROPERTIES UNITY_BUILD ON)
#target_precompile_headers(${MAIN_TARGET} PRIVATE pch.h)