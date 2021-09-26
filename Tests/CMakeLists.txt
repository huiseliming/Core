


add_executable(LoggerTest LoggerTest.cpp)
target_link_libraries(LoggerTest Core)


add_executable(NetworkTest NetworkTest.cpp)
target_link_libraries(NetworkTest Core)

add_executable(NetworkTestClient NetworkTestClient.cpp)
target_link_libraries(NetworkTestClient Core)

add_executable(NetworkTestServer NetworkTestServer.cpp)
target_link_libraries(NetworkTestServer Core)





