cmake_minimum_required(VERSION 3.10)

project(chatServer LANGUAGES CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#mysql-conn相关
#set(mysql-concpp_DIR /usr)
#find_package(mysql-concpp REQUIRED)
find_package(OpenSSL REQUIRED)

set(MYSQL_CPPCONN_LIB "/usr/lib64/libmysqlcppconn.so" CACHE FILEPATH "MySQL Connector/C++ library")
message(STATUS "Using MySQL Connector/C++ library: ${MYSQL_CPPCONN_LIB}")

find_package(jsoncpp REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost 1.82 COMPONENTS system filesystem atomic thread REQUIRED)
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")
set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)
# 添加可执行文件和源文件
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB PBSOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)
add_executable(chatServer ${SOURCES}
        ${PBSOURCES})
target_link_libraries(chatServer
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    Boost::filesystem       
    Boost::system
    Boost::atomic
    Boost::thread
    jsoncpp
    hiredis
    redis++
    ${MYSQL_CPPCONN_LIB}
    OpenSSL::SSL
    OpenSSL::Crypto
    )
    
#复制config.ii文件到build 目录
set(Targetconfig ${CMAKE_CURRENT_SOURCE_DIR}/src/config.ii)
set(OutPutDir ${CMAKE_BINARY_DIR})
add_custom_command(TARGET chatServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${Targetconfig}" "${OutPutDir}/config.ii")
