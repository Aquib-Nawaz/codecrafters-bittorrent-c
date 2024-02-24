cmake_minimum_required(VERSION 3.26)
project(codecrafters_bittorrent_c C)

set(CMAKE_C_STANDARD 11)
add_executable(codecrafters_bittorrent_c
        app/main.c
        app/decoder.c
        app/teensy-sha1.c
        app/command.c)


find_package (CURL)
if (CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
    target_link_libraries(codecrafters_bittorrent_c ${CURL_LIBRARIES})
else()
    MESSAGE(FATAL_ERROR "LibCurl is not found in your system.")
endif (CURL_FOUND)

target_link_libraries(codecrafters_bittorrent_c ${LIBS})