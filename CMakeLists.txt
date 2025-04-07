cmake_minimum_required(VERSION 3.5)
project(Echo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")


add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/logger)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/client)

add_executable(server server.cpp)
target_link_libraries(server logger)
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(echo_client client.cpp)
target_link_libraries(echo_client logger)
target_link_libraries(echo_client client)
target_include_directories(echo_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


add_executable(client_test client_test.cpp)
target_link_libraries(client_test logger)
target_link_libraries(client_test client)
target_include_directories(client_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
