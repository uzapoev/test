cmake_minimum_required(VERSION 2.8)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(WIN32)    
#    include_directories("$ENV{DXSDK_DIR}/Include")
    include_directories("$ENV{QTSDK_DIR}/Include")
    include_directories("$ENV{FBXSDK_DIR}/Include")

    include_directories("$ENV{VULKAN_SDK}/Include")
    include_directories("$ENV{POWERVR_SDK}/Builds/Include")

#    link_directories("$ENV{DXSDK_DIR}/Lib/x86")
    link_directories("$ENV{QTSDK_DIR}/Lib")
    link_directories("$ENV{FBXSDK_DIR}/lib")
    link_directories("$ENV{VULKAN_SDK}")

    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
endif(WIN32)

#add_definitions("-std=c++11")

#set(CMAKE_C_COMPILER "emcc") 

include_directories(./external)
aux_source_directory(./src SOURCES)
aux_source_directory(./src/assets SOURCES)

add_executable(${PROJECT_NAME} ${SOURCES} )