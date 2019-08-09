cmake_minimum_required(VERSION 3.8)
project(videoTrans)

set(CMAKE_CXX_STANDARD 11)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenCV 4 REQUIRED)
find_package(Boost COMPONENTS system filesystem thread REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(CURLPP REQUIRED curlpp)

include_directories(${Boost_INCLUDE_DIR})

include_directories(modules/paho.mqtt.cpp/src)

add_subdirectory(modules/faceDetector)
add_subdirectory(modules/paho.mqtt.cpp)

set(SOURCE_FILES main.cpp videoProcessing.hpp utils.hpp dataProcessing.hpp Stereo.hpp)
add_executable(videoTrans ${SOURCE_FILES})

target_link_libraries(videoTrans ${CURL_LIBRARIES})
target_link_libraries(videoTrans ${CURLPP_LIBRARIES})
target_link_libraries(videoTrans ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(videoTrans ${OpenCV_LIBS})
target_link_libraries(videoTrans ${Boost_SYSTEM_LIBRARY})
target_link_libraries(videoTrans paho-mqttpp3)
target_link_libraries(videoTrans faces)