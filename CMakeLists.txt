cmake_minimum_required(VERSION 3.5)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project(gittest)


if(WIN32)    
    find_package(Vulkan REQUIRED)

#    include_directories("$ENV{DXSDK_DIR}/Include")
    include_directories("$ENV{POWERVR_SDK}/Builds/Include")
#    link_directories("$ENV{DXSDK_DIR}/Lib/x86")

    # Qt
    include_directories("$ENV{QTSDK_DIR}/Include")
    link_directories("$ENV{QTSDK_DIR}/Lib")
    
    # FBXSDK
    include_directories("$ENV{FBXSDK_DIR}/Include")
    link_directories("$ENV{FBXSDK_DIR}/lib")
    
    #dawn(webgpu)
    set(WEBGPU_LIB_PATH, "external/dawn")
    include_directories("${WEBGPU_LIB_PATH}/Include")
    link_directories("${WEBGPU_LIB_PATH}/lib")
    
    # Vulkan
    include_directories("$ENV{VULKAN_SDK}/Include")
    link_directories("$ENV{VULKAN_SDK}")
    
    # Dawn library for webgpu
    include_directories("external/dawn/inc")
    link_directories("external/dawn/bin/win")

    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
 
    set(SDK_FBX_PATH $ENV{FBXSDK_DIR})
    set(VS_VERSIONS vs2013 vs2014 vs2015 vs2017 vs2018 vs2019)
    foreach(vsversion IN LISTS VS_VERSIONS)
      if(EXISTS $ENV{FBXSDK_DIR}/lib/${vsversion})
         set(FBX_LIB_PATH $ENV{FBXSDK_DIR}/lib/${vsversion})
      endif()
    endforeach()
    
    if(EXISTS ${FBX_LIB_PATH})
      execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FBX_LIB_PATH}/x64/debug/libfbxsdk.dll ${PROJECT_BINARY_DIR} )
    endif()

    if(EXISTS ${WEBGPU_LIB_PATH})
    message("fuckfuckfuckfuckfuckfuckfuckfuckfuck")
    endif()

   # execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FBX_LIB_PATH}/x64/debug/libfbxsdk.dll ${PROJECT_BINARY_DIR} )
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WEBGPU_LIB_PATH}/x64/debug/webgpu.dll ${PROJECT_BINARY_DIR} )
    
    # PROJECT_BINARY_DIR PROJECT_SOURCE_DIR
   # message(${PROJECT_BINARY_DIR})

   # set(ADDITIONAL_LIBS )
endif(WIN32)


macro(append_linker_flags FLAGS)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAGS}")
endmacro()

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    
    append_linker_flags("-sFULL_ES3=1")
    append_linker_flags("-sUSE_WEBGPU=1")
    append_linker_flags("-sWASM=1")
    append_linker_flags("-sASYNCIFY") 
    append_linker_flags("-lGL")

 #   set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "--preload-file ../data")
endif(EMSCRIPTEN)


#add_definitions("-std=c++17")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#set(CMAKE_C_COMPILER "emcc") 

function(make_source_group SRC_LIST)
  set( _SRC_LIST ${SRC_LIST} ${ARGN} )
  foreach(source IN LISTS _SRC_LIST)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" source_path0 "${source_path}")
    string(REPLACE ".." "" source_path1 "${source_path0}")
    string(REPLACE "/" "\\" source_path_msvc "${source_path1}") 
    source_group("${source_path_msvc}" FILES "${source}")
  endforeach()
endfunction()
 

include_directories(external)
aux_source_directory(external EXTERNAL_SRCS)

file(GLOB_RECURSE HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
file(GLOB_RECURSE SOURCES_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

set(SOURCES
    src/assets/asset_shader.cpp  
    src/assets/asset_shader.h  
    src/main.cpp
    src/platform/platform_emscripten.cpp  
    src/platform/platform_windows.cpp  
    src/platform/platform.h
    src/gfx/gfx.h
    src/gfx/gfx.cpp
    src/gfx/gfx_memory.h
    src/gfx/gfx_vulkan.cpp
    src/gfx/gfx_webgpu.cpp
    src/scene/scene.cpp
    src/scene/scene.h
    src/gason.cpp
    src/gason.h
    src/json_serializer.h
    src/resource_loader.cpp
    src/resource_loader.h
    src/resource_manager.cpp
    src/resource_manager.h
    src/common.cpp
    src/common.h
    src/memmgr.cpp
    src/memmgr.h
)


#list(APPEND SOURCES ${SOURCES_FILES} ${HEADER_FILES} ${EXTERNAL_SRCS})
#[[
if(WIN32)
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MACOSX_BUNDLE_ICON} ${MACOSX_BUNDLE_RESOURCES})
elseif(APPLE)
endif()
]]

make_source_group(${SOURCES})
message(${PROJECT_BINARY_DIR})
add_executable(${PROJECT_NAME} ${SOURCES} )


if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FBX_LIB_PATH}/x64/release/libfbxsdk.dll" "${PROJECT_BINARY_DIR}" VERBATIM)
elseif (EMSCRIPTEN)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "--preload-file ../data")
endif()