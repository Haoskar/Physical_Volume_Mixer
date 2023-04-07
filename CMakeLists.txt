set(PROJECT_NAME "Physical_Volume_Mixer")
project(Physical_Volume_Mixer)

cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(Header_Files
    "libserialport.h"
    "ProgramVolume.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "ProgramVolume.cpp"
    "TestVolume_c++.cpp"
)
source_group("Source Files" FILES ${Source_Files})


set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
)

set(DLL_DIR "dll_include")

add_executable(${PROJECT_NAME} ${ALL_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    )


target_link_libraries(${PROJECT_NAME} PRIVATE "libserialport")

target_link_directories(${PROJECT_NAME} PRIVATE
    "C:/Users/Oskar/source/repos/Physical_Volume_Mixer/include"
    )

install(
    TARGETS ${PROJECT_NAME}
    DESTINATION .
    )

install(
    FILES dll_include/libserialport.dll
    DESTINATION .
    )

include_directories("${CMAKE_SOURCE_DIR}")
