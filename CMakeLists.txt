cmake_minimum_required(VERSION 3.12)
project(ndnfs_server)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES ndnfs-server.cpp filehandle.cpp connect.cpp)

add_executable(ndnfs_server ${SOURCE_FILES})

include_directories(/usr/include)
link_directories(/usr/lib)

target_link_libraries(ndnfs_server sqlite3 jsoncpp)
