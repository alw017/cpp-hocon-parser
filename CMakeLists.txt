cmake_minimum_required(VERSION 3.7)

if(${CMAKE_VERSION} VERSION_LESS 3.29)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.29)
endif()

project(HOCONParser VERSION 1.0
	DESCRIPTION "HOCON Parser Prototype"
	LANGUAGES CXX)

add_subdirectory(src)
add_executable(main src/main.cpp)
add_executable(tests src/test.cpp)

find_package( CURL REQUIRED )
target_link_libraries( parser CURL::libcurl )

find_package(Catch2 3 REQUIRED)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

target_link_libraries(reader lexer)
target_link_libraries(parser lexer)
target_link_libraries(reader parser)
target_link_libraries(main reader)
target_link_libraries(main parser)
target_link_libraries(tests PUBLIC reader)
target_link_libraries(tests PUBLIC parser)
target_link_libraries(tests PUBLIC lexer)
