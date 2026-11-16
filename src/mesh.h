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



#include <string.h> // memcpy

#if !defined(__ARM_NEON) && !defined(__F16C__) && !(defined(_MSC_VER) && defined(__AVX2__))
// Scalar IEEE-754 float <-> half (round-to-nearest-even), used when there is no hardware conversion.
inline uint16_t float_to_half_scalar(float f)
{
    uint32_t x; memcpy(&x, &f, 4);
    uint32_t sign = (x >> 16) & 0x8000u;
    uint32_t mant = x & 0x007FFFFFu;
    int32_t  exp  = (int32_t)((x >> 23) & 0xFF) - 127 + 15;
    if (((x >> 23) & 0xFF) == 0xFF)                          // inf / nan
        return (uint16_t)(sign | 0x7C00u | (mant ? 0x200u : 0u));
    if (exp >= 0x1F) return (uint16_t)(sign | 0x7C00u);      // overflow -> inf
    if (exp <= 0) {                                           // subnormal / zero
        if (exp < -10) return (uint16_t)sign;
        mant |= 0x00800000u;
        uint32_t shift = (uint32_t)(14 - exp);
        uint32_t h = mant >> shift;
        uint32_t rem = mant & ((1u << shift) - 1u), half = 1u << (shift - 1);
        if (rem > half || (rem == half && (h & 1u))) h++;
        return (uint16_t)(sign | h);
    }
    uint32_t h = sign | ((uint32_t)exp << 10) | (mant >> 13);
    uint32_t rem = mant & 0x1FFFu;
    if (rem > 0x1000u || (rem == 0x1000u && (h & 1u))) h++;  // may carry into exponent: correct behaviour
    return (uint16_t)h;
}

inline float half_to_float_scalar(uint16_t h)
{
    uint32_t sign = (uint32_t)(h & 0x8000u) << 16;
    uint32_t exp  = (h >> 10) & 0x1Fu;
    uint32_t mant = h & 0x3FFu;
    uint32_t x;
    if (exp == 0x1F)      x = sign | 0x7F800000u | (mant << 13);
    else if (exp != 0)    x = sign | ((exp + 112u) << 23) | (mant << 13);
    else if (mant == 0)   x = sign;
    else {                                                    // subnormal -> normalize
        exp = 113;
        while ((mant & 0x400u) == 0) { mant <<= 1; exp--; }
        x = sign | (exp << 23) | ((mant & 0x3FFu) << 13);
    }
    float f; memcpy(&f, &x, 4);
    return f;
}
#endif

inline half4 float4_to_half4(float4 v)
{
    half4 result;
#if defined(__ARM_NEON)
    float16x4_t h = vcvt_f16_f32(vld1q_f32(&v.x));
    vst1_u16(&result.x, vreinterpret_u16_f16(h));
#elif defined(__F16C__) || (defined(_MSC_VER) && defined(__AVX2__))
    __m128i h = _mm_cvtps_ph(_mm_loadu_ps(&v.x), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
    _mm_storel_epi64((__m128i*)&result, h);
#else
    result.x = float_to_half_scalar(v.x);
    result.y = float_to_half_scalar(v.y);
    result.z = float_to_half_scalar(v.z);
    result.w = float_to_half_scalar(v.w);
#endif
    return result;
}

inline float4 half4_to_float4(half4 v)
{
    float4 result;
#if defined(__ARM_NEON)
    float32x4_t f = vcvt_f32_f16(vreinterpret_f16_u16(vld1_u16(&v.x)));
    vst1q_f32(&result.x, f);
#elif defined(__F16C__) || (defined(_MSC_VER) && defined(__AVX2__))
    __m128 f = _mm_cvtph_ps(_mm_loadl_epi64((const __m128i*)&v));
    _mm_storeu_ps(&result.x, f);
#else
    result.x = half_to_float_scalar(v.x);
    result.y = half_to_float_scalar(v.y);
    result.z = half_to_float_scalar(v.z);
    result.w = half_to_float_scalar(v.w);
#endif
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