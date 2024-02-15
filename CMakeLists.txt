cmake_minimum_required(VERSION 3.23)
project(BS3Bot)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${SOURCE_DIR}/include)
include_directories(${SOURCE_DIR}/external/pugixml)

add_executable(BS3Bot ${SOURCE_DIR}/main.cpp ${SOURCE_DIR}/Data/Content.cpp ${SOURCE_DIR}/include/Content.h ${SOURCE_DIR}/Data/Managers.cpp ${SOURCE_DIR}/include/Managers.h ${SOURCE_DIR}/Utils/Utils.cpp ${SOURCE_DIR}/include/Utils.h ${SOURCE_DIR}/Debug/Debugging.cpp ${SOURCE_DIR}/include/Debugging.h)

target_sources(BS3Bot PRIVATE ${SOURCE_DIR}/external/pugixml/pugixml.cpp)

add_custom_command(TARGET BS3Bot POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:BS3Bot>/resources)