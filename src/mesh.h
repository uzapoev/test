#ifndef __mesh_h__
#define __mesh_h__

#include "mathlib.h"

// 512 bits, 64 bytes
struct vertex
{
    vec4    position;   // 128
    vec4    normal;     // 128
    vec4    tangent;    // 128
    vec4    uv;         // 128
};

struct vertex_bones
{
    float4  weight;
    int4    indexes;
};


static_assert(sizeof(uint64_t) == sizeof(half4));

// 192 bits, 24 bytes
typedef struct vertex_packed {
    half4       position;   //64
    half4       uv01;       //64
    uint32_t    tbn;        //32
    uint32_t    color;      //32
} vertex_packed;

typedef struct packed_skinned_vertex : vertex_packed {
    int64_t     index_weight;
} packed_skinned_vertex;



inline half4 float4_to_half4(float4 v) 
{
#ifdef __ARM_NEON
    float16x4_t f16_vec = vcvt_f16_f32(v);
    return vreinterpret_u16_f16(f16_vec);
#else
    half4 result = {};
 //   __m128 vfloat = _mm_set_ps(v.x, v.y, v.z, v.w);
 //   __m128i vhalf = _mm_cvtps_ph(vfloat, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
 //   _mm_storel_epi64((__m128i*) & result, vhalf);
    return result;
#endif
}

inline float4 half4_to_float4(half4 v) 
{
    float4 result = {};
    __m128i in = _mm_loadl_epi64((const __m128i*) & v);
    __m128 f = _mm_cvtph_ps(in);
    _mm_storeu_ps((float*)&result, f);
    return result;
}

static void pack_verts(vertex * src, vertex_packed * dst, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        vec3 t = math::make_vec3(src[i].tangent);
        vec3 n = math::make_vec3(src[i].normal);
        vec3 b = math::cross(t, n);

        quat q = math::encode_tbn(t, b, n);

        dst[i].position = float4_to_half4(*((float4*)&src[i].position));
        dst[i].uv01     = float4_to_half4(*((float4*)&src[i].uv));
        dst[i].tbn      = math::pack_quat(q);

        auto qun = math::unpack_quat(dst[i].tbn);
    }
}

#endif