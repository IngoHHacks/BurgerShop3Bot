cmake_minimum_required(VERSION 3.23)
project(BS3Bot)

set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)
add_executable(BS3Bot WIN32 ${SOURCE_DIR}/main.cpp ${SOURCE_DIR}/Data/Content.cpp include/Content.h ${SOURCE_DIR}/Data/Managers.cpp include/Managers.h ${SOURCE_DIR}/Utils/Utils.cpp include/Utils.h ${SOURCE_DIR}/Debug/Debugging.cpp include/Debugging.h)

target_link_libraries(BS3Bot gdiplus)

add_custom_command(TARGET BS3Bot POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:BS3Bot>/resources)