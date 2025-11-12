cmake_minimum_required(VERSION 3.10)
project(EVCS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加源文件
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/Subject.cpp
    src/Instruction.cpp
    src/AudioPlayer.cpp
    src/ConfigManager.cpp
)

# 添加头文件
set(HEADERS
    src/MainWindow.h
    src/Subject.h
    src/Instruction.h
    src/AudioPlayer.h
    src/ConfigManager.h
)

# 添加资源文件
set(RESOURCES
    resource/resources.rc
)

# 创建可执行文件
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

# 包含资源头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE resource)

# 使用 Unicode 字符集
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    UNICODE 
    _UNICODE 
    _CRT_SECURE_NO_WARNINGS
)

# 设置链接器选项以禁用默认清单生成
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /utf-8 
        /W4
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/MANIFEST:NO"
    )
endif()

# 使用 Unicode 字符集
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

# 链接所需的Windows库
target_link_libraries(${PROJECT_NAME} PRIVATE
    winmm
    comctl32
)
