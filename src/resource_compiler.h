#ifndef __resource_compiler_h__
#define __resource_compiler_h__

#include "resource_manager.h"


// return job. for parallel resource compoiling such as textures, shaders...
// if returned null - resource already compiled.
//static task* resource_compile_xxx(const char* src, const char* dst, const char* meta, platform_type type, void* userdata)
//{
//    bool can_compile_in_task = ...;
// 
//   if(can_compile_in_task) {
//      while(!thread_manager::can_create_task())
//         yield();
// 
//      return thread_manager::create_task([]{
//            ...
//      });
//   }
// 
//    return nullptr;
//}

bool resource_compile_mesh(const char * src, const char * dst, const char * meta, platform_type type, void* userdata);

bool resource_compile_shader(const char * src, const char * dst, const char * meta, platform_type type, void* userdata);

bool resource_compile_texture(const char * src, const char * dst, const char * meta, platform_type type, void* userdata);

bool resource_compile_material(const char * src, const char * dst, const char * meta, platform_type type, void* userdata);

bool resource_compile_scene(const char * src, const char * dst, const char * meta, platform_type type, void* userdata);

#endif 
