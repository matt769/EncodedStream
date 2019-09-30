cmake_minimum_required(VERSION 2.8)
project(EncodedStream)

set(CMAKE_CXX_STANDARD 11)
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
ENDIF()
SET(CMAKE_CXX_FLAGS "-Wall -O2")

add_executable(ex1_send_and_receive_reply
		ex1_send_and_receive_reply.cpp 
		COBS.cpp	
		)

