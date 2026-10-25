cmake_minimum_required(VERSION 4.0.0)
project(KoboldKeysd)

find_package(PkgConfig REQUIRED)
add_executable(koboldkeysd src/main.c src/socket.c src/config.c)
add_executable(testclient examples/client.c)
pkg_check_modules(INI REQUIRED inih)
target_include_directories(koboldkeysd PUBLIC ${INI_INCLUDE_DIRS})
target_link_libraries(koboldkeysd PUBLIC ${INI_LIBRARIES})
