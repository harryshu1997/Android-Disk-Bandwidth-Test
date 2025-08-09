cmake_minimum_required(VERSION 3.10)

# 项目名称
project(AndroidDiskBandwidthTest VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译器特定选项
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
endif()

# 为Android构建设置
if(ANDROID)
    # Android NDK特定设置
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    
    # 链接Android日志库（如果需要）
    find_library(log-lib log)
    
    # Android平台特定的编译标志
    add_definitions(-DANDROID_PLATFORM)
endif()

# 包含头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 创建可执行文件
add_executable(disk_bandwidth_test test.cpp)

# 链接系统库
target_link_libraries(disk_bandwidth_test)

# 如果是Android平台，链接额外的库
if(ANDROID)
    target_link_libraries(disk_bandwidth_test ${log-lib})
endif()

# 设置输出目录
set_target_properties(disk_bandwidth_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 安装规则
install(TARGETS disk_bandwidth_test
    RUNTIME DESTINATION bin
)

# 为交叉编译Android提供额外配置
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    # 确保使用正确的Android API级别
    if(NOT ANDROID_PLATFORM)
        set(ANDROID_PLATFORM android-21)
    endif()
    
    # 设置Android特定的链接器标志
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

# 打印构建信息
message(STATUS "Building for platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
if(ANDROID)
    message(STATUS "Android ABI: ${ANDROID_ABI}")
    message(STATUS "Android Platform: ${ANDROID_PLATFORM}")
endif()
