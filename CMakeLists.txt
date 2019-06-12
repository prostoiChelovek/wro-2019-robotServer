cmake_minimum_required(VERSION 3.8)
project(videoTrans)

set(CMAKE_CXX_STANDARD 17)

set(FACEDEETECTOR_SOURCES modules/faceDetector/FaceRecognizer.cpp modules/faceDetector/FaceRecognizer.h)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Boost COMPONENTS system filesystem thread REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(CURLPP REQUIRED curlpp)

include_directories(${Boost_INCLUDE_DIR})

set(SOURCE_FILES main.cpp videoProcessing.hpp utils.hpp dataProcessing.hpp thingworx.hpp)
set(SOURCE_FILES ${SOURCE_FILES} ${FACEDEETECTOR_SOURCES} speechParser.hpp)
add_executable(videoTrans ${SOURCE_FILES})

target_link_libraries(videoTrans ${CURL_LIBRARIES})
target_link_libraries(videoTrans ${CURLPP_LIBRARIES})
target_link_libraries(videoTrans ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(videoTrans ${OpenCV_LIBS})
target_link_libraries(videoTrans ${Boost_SYSTEM_LIBRARY})