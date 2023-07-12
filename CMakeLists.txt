project(ClariusOpenIGTLinkBridge)
cmake_minimum_required(VERSION 3.2.0)
message(STATUS "> Configuring ${PROJECT_NAME}")

add_executable(ClariusOpenIGTLinkBridge main.cpp SafeQueue.h)
set_property(TARGET ClariusOpenIGTLinkBridge PROPERTY CXX_STANDARD 17)

add_subdirectory(OpenIGTLink)
target_link_libraries(ClariusOpenIGTLinkBridge PUBLIC OpenIGTLink)

set(CAST_VERSION 10.3.0)

set_property(
        SOURCE main.cpp
        APPEND PROPERTY COMPILE_DEFINITIONS CLARIUS_CAST_VERSION="${CAST_VERSION}"
)

if (WIN32)
    set(CLARIUS_CAST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Cast/${CAST_VERSION}/windows" CACHE PATH "Path to the Clarius Cast SDK" FORCE)
    target_link_libraries(ClariusOpenIGTLinkBridge PRIVATE ${CLARIUS_CAST_PATH}/lib/cast.lib)
else ()
    execute_process(COMMAND lsb_release -rs OUTPUT_VARIABLE UbuntuVersion OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (UbuntuVersion VERSION_LESS_EQUAL "20.04")
        set(LINUX_FOLDER_NAME "ubuntu20")
    else ()
        set(LINUX_FOLDER_NAME "ubuntu22")
    endif ()
    set(CLARIUS_CAST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Cast/${CAST_VERSION}/${LINUX_FOLDER_NAME}" CACHE PATH "Path to the Clarius Cast SDK")
    target_link_libraries(ClariusOpenIGTLinkBridge PRIVATE ${CLARIUS_CAST_PATH}/lib/libcast.so)
endif ()
target_include_directories(ClariusOpenIGTLinkBridge PRIVATE ${CLARIUS_CAST_PATH}/include)


