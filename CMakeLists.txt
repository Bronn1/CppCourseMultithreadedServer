cmake_minimum_required(VERSION 2.8)

project(final)

#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#set(CMAKE_CXX_FLAGS_RELEASE "-O3" )

set(LIBS "-pthread")

aux_source_directory(src SRCS)
#set(Source_files "src/main.cpp")
#source_group("Source Files" FILES ${Source_files})

#set(ALL_FILES ${Source_files} )

add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME} ${LIBS})
