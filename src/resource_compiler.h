#ifndef __resource_compiler_h__
#define __resource_compiler_h__

#include "resource_manager.h"


// return job. for parallel resource compoiling such as textures, shaders...
// if returned null - resource already compiled.
//static task_handle_t resource_compile_xxx(const char* src, const char* dst, const char* meta, platform_type type, void* userdata)
//{
//    bool async_compile = ...;
// 
//   if(async_compile) {
//      while(!thread_manager::try_submit([]{....
//          } ))
//         thread_manager::yield();
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
