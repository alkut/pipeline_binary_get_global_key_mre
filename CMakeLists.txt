cmake_minimum_required(VERSION 3.31.6)
project(pipeline_binary_get_global_key_mre LANGUAGES CXX ASM_MASM)

set(CMAKE_CXX_STANDARD 20)

add_executable(pipeline_binary_get_global_key_mre 
	main.cpp
	get_rdi.asm
)

find_package(Vulkan REQUIRED)

target_include_directories(pipeline_binary_get_global_key_mre PRIVATE ${Vulkan_INCLUDE_DIR})
