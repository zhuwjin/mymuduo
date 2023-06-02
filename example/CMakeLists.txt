add_executable(echo_server echo/echo_server.cc)
target_link_libraries(echo_server mymuduo)

add_executable(echo_client echo/echo_client.cc)
target_link_libraries(echo_client mymuduo)