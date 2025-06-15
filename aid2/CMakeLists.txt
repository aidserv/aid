cmake_minimum_required(VERSION 3.12)

project(aid2)
set(CMAKE_CXX_STANDARD 17)    # 使用 C++17 标准
# 告诉 CMake 优先查找静态库
set(OPENSSL_USE_STATIC_LIBS TRUE)
set(BROTLI_USE_STATIC_LIBS TRUE)

add_compile_definitions (HAVE_OPENSSL)
add_compile_definitions (PACKAGE_STRING="$(ProjectName)")
add_compile_definitions (AID_HTTP)

include(FetchContent)
FetchContent_Declare(
  cpp-httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.3 # 使用特定版本
)
FetchContent_MakeAvailable(cpp-httplib)
find_package(nlohmann_json REQUIRED)



set(LIBRARY_SOURCES
    aid2.cpp 
    Afsync.cpp 
    ATH.cpp 
    iOSApplication.cpp 
    iOSDeviceInfo.cpp  
    Logger.cpp 
    RemoteAuth.cpp 
    cpp-base64/cpp-base64.cpp
)


############################################################
# Create a library
############################################################
add_library(${PROJECT_NAME} ${LIBRARY_SOURCES})


#为这个库目标，添加头文件路径，PUBLIC表示包含了这个库的目标也会包含这个路径
target_include_directories(${PROJECT_NAME}
PRIVATE 
        ${PROJECT_SOURCE_DIR} {PROJECT_SOURCE_DIR}/cpp-base64
)

target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PRIVATE httplib)
