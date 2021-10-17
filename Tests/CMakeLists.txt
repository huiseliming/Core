


add_executable(TestLogger TestLogger.cpp)
target_link_libraries(TestLogger Core)


add_executable(TestNetwork TestNetwork.cpp)
target_link_libraries(TestNetwork Core)

add_executable(TestNetworkClient TestNetworkClient.cpp)
target_link_libraries(TestNetworkClient Core)

add_executable(TestNetworkServer TestNetworkServer.cpp)
target_link_libraries(TestNetworkServer Core)

add_executable(TestThreadContext TestThreadContext.cpp)
target_link_libraries(TestThreadContext Core)


