PROJECT(sync_tool)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fPIC"
)

set (sync_client_srcs
    dir_watcher.cpp
    main.cpp
    util.cpp)

add_executable(sync_tool_client ${sync_client_srcs})
target_link_libraries(sync_tool_client pthread)