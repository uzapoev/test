cmake_minimum_required(VERSION 3.5)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project(gittest)


option(USE_VULKAN "Use Vulcan SDK in project" ON)
option(USE_WEBGPU_DAWN "Use dawn lib for webgpu in project" ON)

option(USE_QT "Use Qt SDK in project" OFF)
option(USE_FBX_SDK "Use FBXSDK in project" OFF)

if(WIN32)    
    find_package(Vulkan REQUIRED)
    
    if(USE_QT) # Qt
        include_directories("$ENV{QTSDK_DIR}/Include")
        link_directories("$ENV{QTSDK_DIR}/Lib")
    endif()
    
    if(USE_FBX_SDK) # FBXSDK
        include_directories("$ENV{FBXSDK_DIR}/Include")
        link_directories("$ENV{FBXSDK_DIR}/lib")
        
        if(EXISTS ${FBX_LIB_PATH})
          execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FBX_LIB_PATH}/x64/debug/libfbxsdk.dll ${PROJECT_BINARY_DIR} )
        endif()
    endif()
    
    if(USE_WEBGPU_DAWN)
        set(WEBGPU_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/dawn")
        
        if(EXISTS "${WEBGPU_LIB_PATH}")
            message(STATUS "WebGPU Dawn found at: ${WEBGPU_LIB_PATH}")
            
            include_directories("${WEBGPU_LIB_PATH}/include")
            link_directories("${WEBGPU_LIB_PATH}/bin/win/")
            
            execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WEBGPU_LIB_PATH}/x64/debug/webgpu.dll ${PROJECT_BINARY_DIR} )
            # Automatically copy the DLL after every successful build.
            # $<LOWER_CASE:$<CONFIG>> dynamically resolves to 'debug' or 'release' 
            # based on your current build configuration.
            # Replace 'my_target' with your actual executable/library target name.
         
            # add_custom_command(
            #     TARGET my_target POST_BUILD
            #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
            #     "${WEBGPU_LIB_PATH}/x64/$<LOWER_CASE:$<CONFIG>>/webgpu.dll"
            #     "${PROJECT_BINARY_DIR}"
            #     COMMENT "Copying webgpu.dll to the build directory..."
            # )
        else()
            message(FATAL_ERROR "WEBGPU_LIB_PATH not found at: ${WEBGPU_LIB_PATH}")
        endif()
    endif()
    
    if(USE_VULKAN)# Vulkan
        include_directories("$ENV{VULKAN_SDK}/Include")
        link_directories("$ENV{VULKAN_SDK}")
    endif()

    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
 
    set(SDK_FBX_PATH $ENV{FBXSDK_DIR})
    set(VS_VERSIONS vs2013 vs2014 vs2015 vs2017 vs2018 vs2019)
    foreach(vsversion IN LISTS VS_VERSIONS)
      if(EXISTS $ENV{FBXSDK_DIR}/lib/${vsversion})
         set(FBX_LIB_PATH $ENV{FBXSDK_DIR}/lib/${vsversion})
      endif()
    endforeach()
    


    # PROJECT_BINARY_DIR PROJECT_SOURCE_DIR
   # message(${PROJECT_BINARY_DIR})

   # set(ADDITIONAL_LIBS )
endif(WIN32)


macro(append_linker_flags FLAGS)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAGS}")
endmacro()

if (EMSCRIPTEN)
    add_compile_options(-pthread)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    append_linker_flags("-pthread")
    append_linker_flags("-sSHARED_MEMORY=1")
    append_linker_flags("-sPTHREAD_POOL_SIZE=NAVIGATOR_HARDWARE_CONCURRENCY")
    append_linker_flags("-sPROXY_TO_PTHREAD=1")
    
    append_linker_flags("-sFULL_ES3=1")
    append_linker_flags("-sUSE_WEBGPU=1")
    append_linker_flags("-sWASM=1")
    append_linker_flags("-sASYNCIFY") 
    append_linker_flags("-lGL")

 #   set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "--preload-file ../data")
endif(EMSCRIPTEN)


#add_definitions("-std=c++17")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W3 /Oi /GR- /EHa-) # disable RTTI and exceptions
else()
    add_compile_options(-Wall -Wextra -O3 -fno-rtti -fno-exceptions)
endif()

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

#target_include_directories(gittest SYSTEM PRIVATE /external)

file(GLOB_RECURSE HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
file(GLOB_RECURSE SOURCES_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

set(SOURCES
    src/main.cpp
    src/platform/platform_emscripten.cpp  
    src/platform/platform_windows.cpp  
    src/platform/platform.h
    src/gfx/gfx.h
    src/gfx/gfx_shader_compiler.h
    src/gfx/gfx_shader_reflection.h
    src/gfx/gfx.cpp
    src/gfx/gfx_dx12.h
    src/gfx/gfx_dx12.cpp
    src/gfx/gfx_metal.h
    src/gfx/gfx_vulkan.cpp
    src/gfx/gfx_vulkan.h
    src/gfx/gfx_webgpu.cpp
    src/gfx/gfx_webgpu.h
    src/scene/components.h
    src/scene/scene.cpp
    src/scene/scene.h
    src/scene/scene_json.h
    src/scene/ecs.h
    src/scene/world.h
    src/json_serializer.h
    src/resource_compiler.h
    src/resource_compiler.cpp
    src/resource_loader.cpp
    src/resource_loader.h
    src/resource_manager.cpp
    src/resource_manager.h
    src/common.cpp
    src/common.h
    src/threadpool.cpp
    src/threadpool.h
    src/mathlib.h
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

if(APPLE)
    # macOS + iOS platform layer (Objective-C++, ARC)
    enable_language(OBJCXX)
    list(APPEND SOURCES src/platform/platform_apple.mm)
    set_source_files_properties(src/platform/platform_apple.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
endif()

make_source_group(${SOURCES})
message(${PROJECT_BINARY_DIR})
add_executable(${PROJECT_NAME} ${SOURCES} )


if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FBX_LIB_PATH}/x64/release/libfbxsdk.dll" "${PROJECT_BINARY_DIR}" VERBATIM)
elseif (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Foundation" "-framework QuartzCore" "-framework Metal")
    if(IOS)
        target_link_libraries(${PROJECT_NAME} PRIVATE "-framework UIKit" "-framework UserNotifications")
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa")
    endif()
elseif (EMSCRIPTEN)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "--preload-file ../data")
endif()