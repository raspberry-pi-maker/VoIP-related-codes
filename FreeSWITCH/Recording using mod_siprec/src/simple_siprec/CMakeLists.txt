cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(simple_srs VERSION 1.0)

# Specify the source files
set(SOURCES epoll.cpp main.cpp sip_util.cpp rtp.cpp tinyxml2.cpp util.cpp)

add_executable(simple_srs ${SOURCES} ${COMMON_SOURCES})

# Specify the additional include path
target_include_directories(simple_srs PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/include
        /usr/local/include
)

# Specify the libraries those should be linked
set(MYLIBS pthread uuid stdc++ )
target_link_libraries(simple_srs ${MYLIBS} )


