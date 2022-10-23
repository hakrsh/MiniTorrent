cmake_minimum_required (VERSION 3.16)
project (p2p VERSION 1.0)
set (CMAKE_CXX_FLAGS "-lstdc++fs -std=c++17")
set(CMAKE_BUILD_TYPE Debug)

find_package (glog 0.6.0 REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)


add_executable (client client.cpp)
add_executable (tracker tracker.cpp)

target_link_libraries(client PRIVATE Threads::Threads OpenSSL::Crypto glog::glog)
target_link_libraries(tracker PRIVATE Threads::Threads glog::glog)
