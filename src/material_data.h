#ifndef __material_data_h__
#define __material_data_h__

// Platform independent description of a material: what the .material json holds and what the compiled (binary) material
// contains. The compiler reads it with json::from_json_string, writes it with binary::write_blob; the loader reads it with
// binary::read_blob. One struct, one ReflectObject list - add a field here and both formats follow.

#include "common.h"
#include "mathlib.h"
#include "json_serializer.h"
#include "binary_serializer.h"

ReflectObjectExternal2(vec4, x, y, z, w);

struct material_meta_t {
    interned_string     guid;

    ReflectObject2(material_meta_t, guid);
};

template<class T>
struct shader_slot {
    interned_string     key;            // uniform name
    T                   value = {};

    ReflectObject2(shader_slot, key, value);
};

struct shader_texture_slot {
    guid_t              guid = {};      // texture asset (json: 32 hex digits)
    interned_string     name;           // shader uniform name
    vec4                scaleOffset;

    ReflectObject2(shader_texture_slot, guid, name, scaleOffset);
};

struct material_json {
    interned_string                             shader;     // name or guid, empty = default shader
    std::vector<shader_texture_slot>            textures;
    std::vector<shader_slot<vec4>>              vectors;
    std::vector<shader_slot<float>>             scalars;

    ReflectObject2(material_json, shader, textures, vectors, scalars);
};

// header of a compiled material file
constexpr uint32_t  material_blob_magic   = 'M' | ('A' << 8) | ('T' << 16) | ('B' << 24);
constexpr uint16_t  material_blob_version = 1;

#endif
