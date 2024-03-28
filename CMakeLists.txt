cmake_minimum_required(VERSION 3.27)
project(VWTChunkloadingTestingAndBenching)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -pthread -Wall -pedantic")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(benchmark REQUIRED)
find_package(OpenCV REQUIRED)
find_package(llfio CONFIG REQUIRED)
find_package(OpenSSL 3.0 REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${llfio_INCLUDE_DIRS})
include_directories(${OpenSSL_INCLUDE_DIRS})

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE INCLUDE CONFIGURE_DEPENDS "include/*.h")

add_executable(VWTChunkloadingTestingAndBenching bench.cpp ${SRC} ${INCLUDE})
target_include_directories(VWTChunkloadingTestingAndBenching PRIVATE include)

target_link_libraries(VWTChunkloadingTestingAndBenching PUBLIC Threads::Threads benchmark::benchmark ${OpenCV_LIBS} llfio::hl)
target_link_libraries(VWTChunkloadingTestingAndBenching PUBLIC ${OpenSSL_LIBS} OpenSSL::Crypto OpenSSL::SSL)