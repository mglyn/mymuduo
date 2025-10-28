cmake_minimum_required(VERSION 3.12)
project(mymuduo)

#设置最终编译成的静态库路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 配置C++标准（跨平台，自动适配编译器flag）
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # 强制要求C++20，不 fallback 到低版本
set(CMAKE_CXX_EXTENSIONS OFF)  # 禁用编译器扩展（如GNU扩展）

#设置调试信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++20")

#编译为动态库
aux_source_directory(. SRC_FILES)
add_library(mymuduo SHARED ${SRC_FILES})

