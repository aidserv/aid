cmake_minimum_required(VERSION 3.12)

project(aid3http)
set(CMAKE_CXX_STANDARD 17)    # 使用 C++17 标准

add_compile_definitions (HAVE_OPENSSL)
add_compile_definitions (HAVE_VASPRINTF)
add_compile_definitions (HAVE_ASPRINTF)
add_compile_definitions (HAVE_GETIFADDRS)
add_compile_definitions (HAVE_STPNCPY)
add_compile_definitions (HAVE_STRNDUP)
add_compile_definitions (PACKAGE_STRING="$(ProjectName)")

find_package(nlohmann_json 3.2.0 REQUIRED)

add_subdirectory(aid3/cpp-httplib)
add_subdirectory(aid3/libplist-linux)
add_subdirectory(aid3/libimobiledevice-glue)
add_subdirectory(aid3/libusbmuxd-linux)
add_subdirectory(aid3/libimobiledevice-linux)



############################################################
# Create a library
############################################################
add_library(${PROJECT_NAME} SHARED 
    aid3/aid3.cpp 
    aid3/Afsync.cpp 
    aid3/ATH.cpp 
    aid3/iOSApplication.cpp 
    aid3/iOSDevice.cpp  
    aid3/Logger.cpp 
    aid3/RemoteAuth.cpp 
    aid3/cpp-base64/cpp-base64.cpp
)

#为这个库目标，添加头文件路径，PUBLIC表示包含了这个库的目标也会包含这个路径
target_include_directories(${PROJECT_NAME}
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/aid3 ${PROJECT_SOURCE_DIR}/aid3/cpp-httplib {PROJECT_SOURCE_DIR}/aid3/cpp-base64
)

target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} httplib)
target_link_libraries(${PROJECT_NAME} libplist)
target_link_libraries(${PROJECT_NAME} libimobiledevice-glue)
target_link_libraries(${PROJECT_NAME} libusbmuxd)
target_link_libraries(${PROJECT_NAME} libimobiledevice)


############################################################
# Create an executable
############################################################
#根据cpp.cpp生成可执行文件
add_executable(cpp
    cpp/cpp.cpp
)

target_include_directories(cpp
    PUBLIC 
        ${PROJECT_SOURCE_DIR} 
)

#链接库和可执行文件，使用的是这个库的别名。PRIVATE 表示
target_link_libraries( cpp
    PRIVATE 
        aid3http
)
