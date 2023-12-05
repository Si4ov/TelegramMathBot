cmake_minimum_required(VERSION 3.10.2)
project(echobot)

set(CMAKE_TOOLCHAIN_FILE "C:/src/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(Boost_USE_MULTITHREADED ON)

# Specify the location of OPENSSL and BOOST
set(OPENSSL_ROOT_DIR "C:/src/vcpkg/vcpkg/installed/x64-windows")
set(BOOST_ROOT "C:/src/vcpkg/vcpkg/installed/x64-windows")

# Specify the location of CURL
set(CURL_INCLUDE_DIR "C:/src/vcpkg/vcpkg/installed/x64-windows/include/curl")
set(CURL_LIBRARY "C:/src/vcpkg/vcpkg/installed/x64-windows/lib/libcurl.lib")

# Specify the location of TGBOT
set(TGBOT_LIBRARY "C:/src/vcpkg/vcpkg/installed/x64-windows/lib/TgBot.lib")


find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)
find_package(CURL)
include_directories( ${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIR})
if (CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
    add_definitions(-DHAVE_CURL)
endif()

add_executable(echobot main.cpp)

target_link_libraries(echobot ${TGBOT_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES} ${CURL_LIBRARY})


