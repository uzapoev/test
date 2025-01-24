#ifndef __MATHLIB_H__
#define __MATHLIB_H__

#include <stdlib.h>  // rand
#include <stdint.h>  // int8_t
#include <math.h>
#include <float.h>

#define MATH_INLINE inline

typedef struct vec2     { float x = 0.0f, y = 0.0f; } vec2;
typedef struct vec3     { float x = 0.0f, y = 0.0f, z = 0.0f; } vec3;
typedef struct vec4     { float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;} vec4;

typedef struct float2   { float x = 0.0f, y = 0.0f;                     } float2;
typedef struct float3   { float x = 0.0f, y = 0.0f, z = 0.0f;           } float3;
typedef struct float4   { float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f; } float4;
typedef struct float4x4 { float4 c0, c1, c2, c3;                        } float4x4;

typedef struct short4   { int16_t  x = 0, y = 0, z = 0, w = 0;          } short4;
typedef struct ushort4  { uint16_t x = 0, y = 0, z = 0, w = 0;          } ushort4;
typedef struct int4     { int32_t  x = 0, y = 0, z = 0, w = 0;          } int4;
typedef struct uint4    { uint32_t x = 0, y = 0, z = 0, w = 0;          } uint4;


#define DECL_VPTR       float * v_ptr = (float*)&v.x;
#define DECL_VBPTR      float * v_ptr = (float*)&v.x; float* b_ptr = (float *)&b.x;
#define DECL_D_PTR(T)   T dst; float* dst_ptr = (float*)&dst.x; float* v_ptr = (float *)&v.x;
#define DECL_D_ABPTR(T) T dst; float* dst_ptr = (float*)&dst.x; float* a_ptr = (float *)&a.x; float* b_ptr = (float*)&b.x; 

#define DECLARE_OPERATORS(T, COUNT) \
    MATH_INLINE T operator + (const T& a, const T& b)   { DECL_D_ABPTR(T) for (int i = 0; i < COUNT; ++i) dst_ptr[i] = a_ptr[i] + b_ptr[i]; return dst;}  \
    MATH_INLINE T operator - (const T& a, const T& b)   { DECL_D_ABPTR(T) for (int i = 0; i < COUNT; ++i) dst_ptr[i] = a_ptr[i] - b_ptr[i]; return dst;}  \
    MATH_INLINE T operator * (const T& v, float f)      { DECL_D_PTR(T)   for (int i = 0; i < COUNT; ++i) dst_ptr[i] = v_ptr[i] * f; return dst;}         \
    MATH_INLINE T operator / (const T& v, float f)      { DECL_D_PTR(T)   for (int i = 0; i < COUNT; ++i) dst_ptr[i] = v_ptr[i] / f; return dst;}         \
    MATH_INLINE T operator * (float f, const T& v)      { DECL_D_PTR(T)   for (int i = 0; i < COUNT; ++i) dst_ptr[i] = v_ptr[i] * f; return dst;}         \
    MATH_INLINE T operator / (float f, const T& v)      { DECL_D_PTR(T)   for (int i = 0; i < COUNT; ++i) dst_ptr[i] = v_ptr[i] / f; return dst;}         \
    MATH_INLINE void operator *= (T& v, float f)        { DECL_VPTR     for (int i = 0; i < COUNT; ++i) v_ptr[i] *= f; }        \
    MATH_INLINE void operator /= (T& v, float f)        { DECL_VPTR     for (int i = 0; i < COUNT; ++i) v_ptr[i] /= f; }        \
    MATH_INLINE void operator += (T& v, const T& b)     { DECL_VBPTR    for (int i = 0; i < COUNT; ++i) v_ptr[i] += b_ptr[i]; } \
    MATH_INLINE void operator -= (T& v, const T& b)     { DECL_VBPTR    for (int i = 0; i < COUNT; ++i) v_ptr[i] -= b_ptr[i]; } \

DECLARE_OPERATORS(vec2, 2)
DECLARE_OPERATORS(vec3, 3)
DECLARE_OPERATORS(vec4, 4)

namespace math { bool fcmp(float, float); };

MATH_INLINE bool operator == (const vec2& a, const vec2& b) { return math::fcmp(a.x, b.x) && math::fcmp(a.y, b.y); }
MATH_INLINE bool operator == (const vec3& a, const vec3& b) { return math::fcmp(a.x, b.x) && math::fcmp(a.y, b.y) && math::fcmp(a.z, b.z); }
MATH_INLINE bool operator == (const vec4& a, const vec4& b) { return math::fcmp(a.x, b.x) && math::fcmp(a.y, b.y) && math::fcmp(a.z, b.z) && math::fcmp(a.w, b.w); }

MATH_INLINE vec3 operator - (const vec3& v) { return { -v.x, -v.y, -v.z }; }

static bool g_is_right_hand = false;

static bool is_right_hand()         { return g_is_right_hand;   }
static void set_right_hand(bool rh) { g_is_right_hand = rh;     }

namespace math
{
    static const float Epsilon = 1e-4f;
    static const float Pi = 3.14159265f;
    static const float Pi_2 = 6.28318521f;  // pi*2
    static const float Pi_half = 1.57079633f;  // pi/2

    static const vec3 Up       = { 0.0f,  1.0f,  0.0f};
    static const vec3 Down     = { 0.0f, -1.0f,  0.0f};
    static const vec3 Left     = { 1.0f,  0.0f,  0.0f};
    static const vec3 Right    = {-1.0f,  0.0f,  0.0f};
    static const vec3 Forward  = { 0.0f,  0.0f,  1.0f};
    static const vec3 Backward = { 0.0f,  0.0f, -1.0f};
    static const vec3 Zero     = { 0.0f,  0.0f,  0.0f};
    static const vec3 One      = { 1.0f,  1.0f,  1.0f};


    MATH_INLINE float               deg2rad(float d)                        { return d * (Pi / 180.0f); }

    MATH_INLINE float               rad2deg(float d)                        { return d * (180.0f / Pi); }

    MATH_INLINE bool                fcmp(float a, float b)                  { return fabs(a - b) < math::Epsilon; }

    MATH_INLINE float               clamp(float x, float lo, float hi)      { return fminf(hi, fmaxf(x, lo)); }

    MATH_INLINE float               rsqrt(float x)                          { float l = sqrtf(x); return (l < Epsilon) ? 1.0f : 1.0f / l; }

    template<class T> MATH_INLINE T lerp(const T& a, const T& b, float t)   { return a + ((b - a) * t); }

    MATH_INLINE float               remap(float s, float a1, float a2, float b1, float b2) { 
        return b1 + (s - a1) * (b2 - b1) / (a2 - a1); 
    }

    MATH_INLINE void                vsincos(const vec4& v, vec4* s, vec4* c) {
        s->x = sinf(v.x);   s->y = sinf(v.y);   s->z = sinf(v.z);   s->w = sinf(v.w);
        c->x = cosf(v.x);   c->y = cosf(v.y);   c->z = cosf(v.z);   c->w = cosf(v.w);
    }

    MATH_INLINE vec3 make_vec3(float x, float y, float z)           { return {x,y,z}; }
    MATH_INLINE vec4 make_vec4(const vec3 &a)                       { return { a.x, a.y, a.z, 1.0f}; }
    MATH_INLINE vec4 make_vec4(float x, float y, float z, float w)  { return { x, y, z, w}; }

    MATH_INLINE vec3  cross(const vec3& a, const vec3& b)       { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
    MATH_INLINE vec3  cross(const vec4& a, const vec4& b)       { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }

    MATH_INLINE float dot(const vec3& a, const vec3& b)         { return a.x * b.x + a.y * b.y + a.z * b.z; }
    MATH_INLINE float dot(const vec4& a, const vec4& b)         { return a.x * b.x + a.y * b.y + a.z * b.z; }

    MATH_INLINE float length(const vec2& a)                     { return sqrtf(a.x * a.x + a.y * a.y); }
    MATH_INLINE float length(const vec3& a)                     { return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z); }
    MATH_INLINE float length(const vec4& a)                     { return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z); }

    MATH_INLINE float magnitude_sq(const vec2& a)               { return (a.x * a.x) + (a.y * a.y); }
    MATH_INLINE float magnitude_sq(const vec3& a)               { return (a.x * a.x) + (a.y * a.y) + (a.z * a.z); }
    MATH_INLINE float magnitude_sq(const vec4& a)               { return (a.x * a.x) + (a.y * a.y) + (a.z * a.z); }

    MATH_INLINE float distance(const vec2& a, const vec2& b)    { return length(vec2{a.x - b.x, a.y - b.y}); }
    MATH_INLINE float distance(const vec3& a, const vec3& b)    { return length(vec3{a.x - b.x, a.y - b.y, a.z - b.z}); }
    MATH_INLINE float distance(const vec4& a, const vec4& b)    { return length(vec4{a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }); }

    MATH_INLINE vec2  normalize(const vec2& a)                  { float l = length(a); return { a.x / l, a.y / l }; }
    MATH_INLINE vec3  normalize(const vec3& a)                  { float l = length(a); return { a.x / l, a.y / l, a.z / l }; }
    MATH_INLINE vec4  normalize(const vec4& a)                  { float l = length(a); return { a.x / l, a.y / l, a.z / l, a.w }; }

    MATH_INLINE vec3 reflect(const vec3& v, const vec3& n)      { return v - 2.0f * dot(n, v) * n; }
    MATH_INLINE vec3 project(const vec3& v, const vec3& n)      { return v - (dot(v, n) / magnitude_sq(n)) * n; }
    MATH_INLINE vec3 refract(const vec3& v, const vec3& n, float eta)
    {
        float nv = dot(n, v);
        float k = 1.0f - eta * eta * (1.0f - nv * nv);
        return k < 0.0f ? math::Zero : eta * v - (eta * nv + sqrtf(k)) * n;
    }

    // http://probesys.blogspot.com/2011/10/useful-math-functions.html
    namespace ease
    {
        MATH_INLINE float sine_in(float T)      { return sinf((T - 1.0f) * Pi_half) + 1.0f; }
        MATH_INLINE float sine_out(float T)     { return sinf(T * Pi_half); }
        MATH_INLINE float sine_inout(float T)   { return (0.5f * (1.0f - cosf(T * Pi))); }

        // Quadratic
        MATH_INLINE float quad_in(float T)      { return T * T; }
        MATH_INLINE float quad_out(float T)     { return 0.0f - T * (T - 2.0f); }
        MATH_INLINE float quad_inout(float T)   { return (T < 0.5f) ? (2.0f * T * T) : (-2.0f * T * T + 4.0f * T - 1.0f); }
         
        // Cubic
        MATH_INLINE float cubic_in(float T)     { return T * T * T; }
        MATH_INLINE float cubic_out(float T)    { float F = T - 1.0f; return F * F * F + 1.0f; }
        MATH_INLINE float cubic_inout(float T) 
        {
            if (T < 0.5f)
                return 4.0f * T * T * T;

            float F = 2.0f * T - 2.0f;
            return (float)(0.5f * F * F * F + 1.0f);
        }

        // Quartic
        MATH_INLINE float quart_in(float T)     { return T * T * T * T;   }
        MATH_INLINE float quart_out(float T)    { float F = T - 1.0f; return F * F * F * (1 - T) + 1.0f; }
        MATH_INLINE float quart_inout(float T)
        {
            if (T < 0.5f)
                return 8.0f * T * T * T * T;

            float F = T - 1.0f;
            return -8.0f * F * F * F * F + 1.0f;
        }

        // Quintic
        MATH_INLINE float quint_in(float T)     { return T * T * T * T * T;        }
        MATH_INLINE float quint_out(float T)    { float F = T - 1.0f; return F * F * F * F * F + 1.0f;    }
        MATH_INLINE float quint_inout(float T)
        {
            if (T < 0.5f)
                return 16.0f * T * T * T * T * T;
            float F = 2.0f * T - 2.0f;
            return (float)(0.5f * F * F * F * F * F + 1.0f);
        }

        MATH_INLINE float expo_in(float T)      { return fcmp(T, 0.0f) ? powf(2.0f, 10.0f * (T - 1.0f)) : T; }
        MATH_INLINE float expo_out(float T)     { return fcmp(T, 1.0f) ? T : 1.0f - powf(2.0f, -10.0f * T); }
        MATH_INLINE float expo_inout(float T)
        {
            if (T < Epsilon || T >(1.0f - Epsilon))
                return T;

            return (T < 0.5f) ? (float)(0.5f * powf(2.0f, (20.0f * T) - 10.0f)) :
                (float)(-0.5f * powf(2.0f, (-20.0f * T) + 10.0f) + 1.0f);
        }

        // Circular
        MATH_INLINE float circ_in(float T)      { return 1.0f - sqrtf(1.0f - T * T); }
        MATH_INLINE float circ_out(float T)     { return sqrtf((2.0f - T) * T); }
        MATH_INLINE float circ_inout(float T)
        {
            return (T < 0.5f) ? (0.5f * (1.0f - sqrtf(1.0f - 4.0f * T * T))) :
                                (0.5f * (sqrtf(0.0f - (2.0f * T - 3.0f) * (2.0f * T - 1.0f)) + 1.0f));
        }

        // Back
        MATH_INLINE float back_in(float T)      { return T * T * T - T * sinf(T * Pi); }
        MATH_INLINE float back_out(float T)     { float F = 1.0f - T; return 1.0f - (F * F * F - F * sinf(F * Pi)); }
        MATH_INLINE float back_inout(float T)
        {
            float F0 = 2.0f * T;
            float F1 = 1.0f - (2.0f * T - 1.0f);
            return (T < 0.5f) ? (0.5f * (F0 * F0 * F0 - F0 * sinf(F0 * Pi))) :
                                (0.5f * (1.0f - (F1 * F1 * F1 - F1 * sinf(F1 * Pi))) + 0.5f);
        }

        // Elastic
        MATH_INLINE float elastic_in(float T)   { return sinf(13.0f * Pi_half * T) * powf(2.0, 10.0f * (T - 1.0f)); }
        MATH_INLINE float elastic_out(float T)  { return sinf(-13.0f * Pi_half * (T + 1.0f)) * powf(2.0f, -10.0f * T) + 1.0f; }
        MATH_INLINE float elastic_inout(float T)
        {
            return (T < 0.5f) ? (0.5f * sinf(13.0f * Pi * T) * powf(2.0f, 10.0f * (2.0f * T - 1.0f))) :
                                (0.5f * (sinf(-13.0f * Pi * T) * powf(2.0f, -10.0f * (2.0f * T - 1.0f)) + 2.0f));
        }

        // k controls the stretching of the func
        MATH_INLINE float impulse(float x, float k)             { float h = k * x; return h * expf(1.0f - h); }
        MATH_INLINE float exppulse(float x, float k, float n)   { return expf(-k * powf(x, n)); }
    };  // namespace ease

    namespace quant
    {
        // [ref] http://zeuxcg.org/2010/12/14/quantizing-floats/
        template<unsigned N> inline uint16_t  encode_unorm(float    x)  { return uint16_t(int(x * ((1 << (N)) - 1) + 0.5f)); }
        template<unsigned N> inline float     decode_unorm(uint16_t x)  { return x / float((1 << (N)) - 1); }
        template<unsigned N> inline uint16_t  encode_snorm(float    x)  { return (x < 0) | (encode_unorm<N - 1>(x < 0 ? -x : x) << 1); }
        template<unsigned N> inline float     decode_snorm(uint16_t x)  { return decode_unorm<N - 1>(x >> 1) * (x & 1 ? -1 : 1); }

        inline uint8_t   encode8_unorm(float    x)                      { return uint8_t(int(x * 255.f + 0.5f)); }
        inline float     decode8_unorm(uint8_t  x)                      { return x / 255.f; }
        inline uint8_t   encode8_snorm(float    x)                      { return uint8_t(int(x * 127.f + (x > 0 ? 0.5f : -0.5f))); }
        inline float     decode8_snorm(uint8_t  x)                      { float f = x / 127.f; return f <= -1 ? -1.f : (f >= 1 ? 1.f : f); }

        inline uint16_t  encode16_unorm(float    x)                     { return encode_unorm<16>(x); }
        inline float     decode16_unorm(uint16_t x)                     { return decode_unorm<16>(x); }
        inline uint16_t  encode16_snorm(float    x)                     { return encode_snorm<16>(x); }
        inline float     decode16_snorm(uint16_t x)                     { return decode_snorm<16>(x); }

  //      void            encode_quat(uint32_t& out, float x, float y, float z, float w);
  //      void            decode_quat(float& x, float& y, float& z, float& w, uint32_t in);
  //      void            tbn_to_quat(const float* n, const float* t, const float* b, float * out);
    };

};  // namespace math



//  http://willperone.net/Code/quaternion.php
struct quat
{
    static quat zero()          { return { 0.0f, 0.0f, 0.0f, 0.0f}; }
    static quat identity()      { return { 0.0f, 0.0f, 0.0f, 1.0f}; }

    quat():x(0.0f), y(0.0f), z(0.0f), w(1.0f)                           {}
    quat(float x, float y, float z, float w):x(x), y(y), z(z), w(w)     {}

    static float    dot(const quat &a, const quat &b)                   { return ((a.w * b.w) + (a.x * b.x) + (a.y * b.y) + (a.z * b.z)); }

    static quat     slerp(const quat &a, const quat &b, float t)
    {
        float co = quat::dot(a, b);

        float d = fabsf(co);
        float k = d * (d * 0.331442f - 1.25654f) + 0.931872f;
        float ot = t + t * (t - 0.5f) * (t - 1.0f) * k;
        float lt = 1.0f - ot;
        float rt = co > 0.0f ? ot : -ot;

        return quat(
            (lt * a.x) + (rt * b.x),
            (lt * a.y) + (rt * b.y),
            (lt * a.z) + (rt * b.z),
            (lt * a.w) + (rt * b.w)
        );
    }

    static quat     mul(const quat& a, const quat& b)
    {
        float x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
        float y = a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z;
        float z = a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x;
        float w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;

        return quat(x, y, z, w);
    }

    static vec3     mul(const quat& q, const vec3& p)
    {
        quat tmp = mul(q, quat(p.x, p.y, p.z, 0.0f));
        quat res = mul(tmp, quat(-q.x, -q.y, -q.z, q.w));
        return { res.x, res.y, res.z };
    }

    static vec4     mul(const quat& q, const vec4& p)
    {
        quat tmp = mul(q, quat(p.x, p.y, p.z, 0.0f));
        quat res = mul(tmp, quat(-q.x, -q.y, -q.z, q.w));
        return { res.x, res.y, res.z, 1.0f };
    }

    static quat     from_euler(float roll, float pitch, float yaw)
    {
        vec4 v { math::deg2rad(roll) , math::deg2rad(pitch) , math::deg2rad(yaw), 1.0f };
        vec4 s, c;
        math::vsincos(v * 0.5f, &s, &c);

        return quat{ s.x * c.y * c.z - c.x * s.y * s.z,
                    c.x * s.y * c.z + s.x * c.y * s.z,
                    c.x * c.y * s.z - s.x * s.y * c.z,
                    c.x * c.y * c.z + s.x * s.y * s.z };
    }

    static quat     from_axis(float degrees, const vec3& axis)
    {
        float half = math::deg2rad(degrees) * 0.5f;
        float s = sinf(half);
        float c = cosf(half);
        return quat(axis.x * s, axis.y * s, axis.z * s, c);
    }

public:
    MATH_INLINE  quat  conjugate() const                    { return quat(-x, -y, -z, w); }
    MATH_INLINE  quat  inverted() const                     { return conjugate() / (x * x + y * y + z * z + w * w); }

    MATH_INLINE quat operator * (float scalar) const        { return quat(x * scalar, y * scalar, z * scalar, w * scalar); }
    MATH_INLINE quat operator / (float scalar) const        { return quat(x / scalar, y / scalar, z / scalar, w / scalar); }

    friend quat operator*(const quat &a, const quat &b)     {return quat::mul(a, b);}
    friend quat operator*(const quat &rhs, float s)         { return quat(rhs.x * s, rhs.y * s, rhs.z * s, rhs.w *s); }

public:
    float x, y, z, w;
};


struct mat4
{
    static mat4 zero()      { return { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };}
    static mat4 identity()  { return { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };}

public:
    mat4()                          { *this = mat4::identity(); }

    mat4(float _m00, float _m01, float _m02, float _m03,
         float _m10, float _m11, float _m12, float _m13,
         float _m20, float _m21, float _m22, float _m23,
         float _m30, float _m31, float _m32, float _m33)
         {
                 m00 = _m00; m01 = _m01; m02 = _m02; m03 = _m03;
                 m10 = _m10; m11 = _m11; m12 = _m12; m13 = _m13;
                 m20 = _m20; m21 = _m21; m22 = _m22; m23 = _m23;
                 m30 = _m30; m31 = _m31; m32 = _m32; m33 = _m33;
         }

public:
    static mat4 trs(const vec3 & pos, const quat& rot, const vec3 & scale)
    {
        mat4 r, t, s;
        r = mat4::form_quat(rot);
        t = mat4::translate(pos);
        s = mat4::scale(scale);
        return mat4::mul(mat4::mul(t, r), s);
    }

    static mat4 mul(const mat4& a, const mat4& b)
    {
        return mat4( a.m[0] * b.m[0] + a.m[4] * b.m[1] + a.m[8] * b.m[2] + a.m[12] * b.m[3],        
                     a.m[0] * b.m[4] + a.m[4] * b.m[5] + a.m[8] * b.m[6] + a.m[12] * b.m[7],
                     a.m[0] * b.m[8] + a.m[4] * b.m[9] + a.m[8] * b.m[10] + a.m[12] * b.m[11],
                     a.m[0] * b.m[12] + a.m[4] * b.m[13] + a.m[8] * b.m[14] + a.m[12] * b.m[15],

                     a.m[1] * b.m[0] + a.m[5] * b.m[1] + a.m[9] * b.m[2] + a.m[13] * b.m[3], 
                     a.m[1] * b.m[4] + a.m[5] * b.m[5] + a.m[9] * b.m[6] + a.m[13] * b.m[7],
                     a.m[1] * b.m[8] + a.m[5] * b.m[9] + a.m[9] * b.m[10] + a.m[13] * b.m[11],
                     a.m[1] * b.m[12] + a.m[5] * b.m[13] + a.m[9] * b.m[14] + a.m[13] * b.m[15],

                     a.m[2] * b.m[0] + a.m[6] * b.m[1] + a.m[10] * b.m[2] + a.m[14] * b.m[3],  
                     a.m[2] * b.m[4] + a.m[6] * b.m[5] + a.m[10] * b.m[6] + a.m[14] * b.m[7],
                     a.m[2] * b.m[8] + a.m[6] * b.m[9] + a.m[10] * b.m[10] + a.m[14] * b.m[11],
                     a.m[2] * b.m[12] + a.m[6] * b.m[13] + a.m[10] * b.m[14] + a.m[14] * b.m[15],

                     a.m[3] * b.m[0] + a.m[7] * b.m[1] + a.m[11] * b.m[2] + a.m[15] * b.m[3],
                     a.m[3] * b.m[4] + a.m[7] * b.m[5] + a.m[11] * b.m[6] + a.m[15] * b.m[7],
                     a.m[3] * b.m[8] + a.m[7] * b.m[9] + a.m[11] * b.m[10] + a.m[15] * b.m[11],
                     a.m[3] * b.m[12] + a.m[7] * b.m[13] + a.m[11] * b.m[14] + a.m[15] * b.m[15]
        );
    }

    static vec3 mul(const mat4& m, const vec3& p)
    {
        float x = math::dot(math::make_vec4(p), m.column(0));
        float y = math::dot(math::make_vec4(p), m.column(1));
        float z = math::dot(math::make_vec4(p), m.column(2));
        float w = math::dot(math::make_vec4(p), m.column(3));
        if (fabs(w) < math::Epsilon)
            return math::make_vec3(x, y, z);

        return math::make_vec3(x / w, y / w, z / w);
    }

    static vec4 mul(const mat4& m, const vec4& p)
    {
        float x = math::dot(p, m.column(0));
        float y = math::dot(p, m.column(1));
        float z = math::dot(p, m.column(2));
        float w = math::dot(p, m.column(3));
        return { x, y, z, w };
    }

    static quat to_quat(const mat4& m)
    {
        float trace = m.m00 + m.m11 + m.m22;
        if (trace > 0.0f) {
            float s = sqrtf(trace + 1.0f) * 2.0f; // S=4*qw 
            return quat((m.m21 - m.m12) / s, (m.m02 - m.m20) / s, (m.m10 - m.m01) / s, 0.25f * s);
        }
        else if ((m.m00 > m.m11) && (m.m00 > m.m22)) {
            float s = sqrtf(1.0f + m.m00 - m.m11 - m.m22) * 2.0f; // S=4*qx 
            return quat(0.25f * s, (m.m01 + m.m10) / s, (m.m02 + m.m20) / s, (m.m21 - m.m12) / s);
        }
        else if (m.m11 > m.m22) {
            float s = sqrtf(1.0f + m.m11 - m.m00 - m.m22) * 2.0f; // S=4*qy
            return quat((m.m01 + m.m10) / s, 0.25f * s, (m.m12 + m.m21) / s, (m.m02 - m.m20) / s);
        }
        float s = sqrtf(1.0f + m.m22 - m.m00 - m.m11) * 2.0f; // S=4*qz
        return quat((m.m02 + m.m20) / s, (m.m12 + m.m21) / s, 0.25f * s, (m.m10 - m.m01) / s);
    }

    static mat4 form_quat(const quat& q)
    {
        float x = q.x, y = q.y, z = q.z, w = q.w;
        float x2 = x + x, y2 = y + y, z2 = z + z;
        float xx = x * x2, xy = x * y2, xz = x * z2;
        float yy = y * y2, yz = y * z2, zz = z * z2;
        float wx = w * x2, wy = w * y2, wz = w * z2;

        float inv_yz = 1.0f - (yy + zz);
        float inv_xz = 1.0f - (xx + zz);
        float inv_xy = 1.0f - (xx + yy);

        return mat4(inv_yz,  xy - wz, xz + wy, 0.0f,
                    xy + wz, inv_xz,  yz - wx, 0.0f,
                    xz - wy, yz + wx, inv_xy,  0.0f,
                    0.0f,    0.0f,    0.0f,    1.0f);
    }

    static mat4 form_eulers(float x, float y, float z)
    {
        vec4 v{ math::deg2rad(x), math::deg2rad(y), math::deg2rad(z), 1.0f };

        mat4 m;
        vec4 c, s;
        math::vsincos(v, &s, &c);

        m[0] = (c.y * c.z); m[4] = (s.x * s.y * c.z - c.x * s.z);   m[8] = (c.x * s.y * c.z + s.x * s.z);
        m[1] = (c.y * s.z); m[5] = (s.x * s.y * s.z + c.x * c.z);   m[9] = (c.x * s.y * s.z - s.x * c.z);
        m[2] = (-s.y);      m[6] = (s.x * c.y);                     m[10] = (c.x * c.y);

        m[15] = 1.0f;
        return m;
    }

    static mat4 scale(const vec3& v)
    {
        mat4 m;
        m[0] = v.x; m[5] = v.y; m[10] = v.z; m[15] = 1.0;
        return m;
    }

    static mat4 translate(const vec3& v)
    {
        mat4 m;
        m[12] = v.x; m[13] = v.y; m[14] = v.z;
        return m;
    }

    static mat4 make_ortho_center(float l, float r, float b, float t, float n, float f, bool rh)
    {
        mat4 ret;
        float fn = rh ? (n - f) : (f - n);
        ret.m[0] = 2.0f / (r - l);           ret.m[1] = 0.0f;               ret.m[2] = 0.0f;                ret.m[3] = 0.0f;
        ret.m[4] = 0.0f;                     ret.m[5] = 2.0f / (t - b);     ret.m[6] = 0.0f;                ret.m[7] = 0.0f;
        ret.m[8] = 0.0f;                     ret.m[9] = 0.0f;               ret.m[10] = 1.0f / fn;     ret.m[11] = 0.0f;
        ret.m[12] = (1.0f + r) / (1.0f - r); ret.m[13] = (t + b) / (b - t); ret.m[14] = -n / (f - n);       ret.m[15] = 1.0f;
        return ret;
    }

    static mat4 transpose(const mat4 &m) 
    {
        return mat4(m.m[0],  m.m[1],   m.m[2],   m.m[3],
                    m.m[4],  m.m[5],   m.m[6],   m.m[7],
                    m.m[8],  m.m[9],   m.m[10],  m.m[11],
                    m.m[12], m.m[13],  m.m[14],  m.m[15]);
    }

    static mat4 inverse(const mat4& m)
    {
        float d12 = (m.m20 * m.m31 - m.m30 * m.m21);
        float d13 = (m.m20 * m.m32 - m.m30 * m.m22);
        float d23 = (m.m21 * m.m32 - m.m31 * m.m22);
        float d24 = (m.m21 * m.m33 - m.m31 * m.m23);
        float d34 = (m.m22 * m.m33 - m.m32 * m.m23);
        float d41 = (m.m23 * m.m30 - m.m33 * m.m20);

        mat4 tmp;

        tmp[0] =  (m.m11 * d34 - m.m12 * d24 + m.m13 * d23);
        tmp[1] = -(m.m10 * d34 + m.m12 * d41 + m.m13 * d13);
        tmp[2] =  (m.m10 * d24 + m.m11 * d41 + m.m13 * d12);
        tmp[3] = -(m.m10 * d23 - m.m11 * d13 + m.m12 * d12);

        // Compute determinant as early as possible using these cofactors.
        float det = m.m00 * tmp[0] + m.m01 * tmp[1] + m.m02 * tmp[2] + m.m03 * tmp[3];

        // Run singularity test.
        if (det == 0.0)
            return mat4::identity();

        float invDet = 1.0f / det;

        tmp[0] *= invDet;
        tmp[1] *= invDet;
        tmp[2] *= invDet;
        tmp[3] *= invDet;

        tmp[4] = -(m.m01 * d34 - m.m02 * d24 + m.m03 * d23) * invDet;
        tmp[5] =  (m.m00 * d34 + m.m02 * d41 + m.m03 * d13) * invDet;
        tmp[6] = -(m.m00 * d24 + m.m01 * d41 + m.m03 * d12) * invDet;
        tmp[7] =  (m.m00 * d23 - m.m01 * d13 + m.m02 * d12) * invDet;

        // Pre-compute 2x2 dets for first two rows when computing cofactors of last two rows.
        d12 = m.m00 * m.m11 - m.m10 * m.m01;
        d13 = m.m00 * m.m12 - m.m10 * m.m02;
        d23 = m.m01 * m.m12 - m.m11 * m.m02;
        d24 = m.m01 * m.m13 - m.m11 * m.m03;
        d34 = m.m02 * m.m13 - m.m12 * m.m03;
        d41 = m.m03 * m.m10 - m.m13 * m.m00;
        
        tmp[8]  =  (m.m31 * d34 - m.m32 * d24 + m.m33 * d23) * invDet;
        tmp[9]  = -(m.m30 * d34 + m.m32 * d41 + m.m33 * d13) * invDet;
        tmp[10] =  (m.m30 * d24 + m.m31 * d41 + m.m33 * d12) * invDet;
        tmp[11] = -(m.m30 * d23 - m.m31 * d13 + m.m32 * d12) * invDet;
        
        tmp[12] = -(m.m21 * d34 - m.m22 * d24 + m.m23 * d23) * invDet;
        tmp[13] =  (m.m20 * d34 + m.m22 * d41 + m.m23 * d13) * invDet;
        tmp[14] = -(m.m20 * d24 + m.m21 * d41 + m.m23 * d12) * invDet;
        tmp[15] =  (m.m20 * d23 - m.m21 * d13 + m.m22 * d12) * invDet;
        
        return tmp;
    }

    static void decompose(const mat4& m, vec3& p, quat& r, vec3& s)
    {
        p.x = m.m[12];
        p.y = m.m[13];
        p.z = m.m[14];

        float det = m.m[0] * (m.m[5] * m.m[10] - m.m[6] * m.m[9]) -
                    m.m[1] * (m.m[4] * m.m[10] - m.m[6] * m.m[8]) +
                    m.m[2] * (m.m[4] * m.m[9]  - m.m[5] * m.m[8]);

        s.x = (det < 0 ? -1 : +1) * sqrtf(m.m[0] * m.m[0] + m.m[1] * m.m[1] + m.m[2] * m.m[2]);
        s.y = sqrtf(m.m[4] * m.m[4] + m.m[5] * m.m[5] + m.m[6] * m.m[6]);
        s.z = sqrtf(m.m[8] * m.m[8] + m.m[9] * m.m[9] + m.m[10] * m.m[10]);

        r = mat4::to_quat(m);
    }

public:
    MATH_INLINE float& operator[](size_t i) { return ((float*)this)[i]; }

    MATH_INLINE vec4 row(int i) const                                   { return {m[i * 4 + 0], m[i * 4 + 1], m[i * 4 + 2], m[i * 4 + 3]}; }
    MATH_INLINE vec4 column(int i)const                                 { return {m[i],         m[i + 4],     m[i + 8],     m[i + 12] }; }

    union
    {
        float m[16] = {};
        struct {
            float m00, m10, m20, m30;
            float m01, m11, m21, m31;
            float m02, m12, m22, m32;
            float m03, m13, m23, m33;
        };
    };
};


struct aabbox
{
    aabbox() = default;
    aabbox(const vec3& in_min, const vec3& in_max):
        m_min{ in_min.x, in_min.y, in_min.z, 1.0f },
        m_max{ in_max.x, in_max.y, in_max.z, 1.0f }
        {  }

    MATH_INLINE void extend(const vec3 & p)
    {
        m_min.x = fminf(p.x, m_min.x); m_max.x = fmaxf(p.x, m_max.x);
        m_min.y = fminf(p.y, m_min.y); m_max.y = fmaxf(p.y, m_max.y);
        m_min.z = fminf(p.z, m_min.z); m_max.z = fmaxf(p.z, m_max.z);
    }

    MATH_INLINE vec3 center() const
    {
        return  {(m_min.x + m_max.x) * 0.5f,
                 (m_min.y + m_max.y) * 0.5f,
                 (m_min.z + m_max.z) * 0.5f};
    }

    MATH_INLINE vec3 extends() const
    {
        auto c = center();
        return  {(m_max.x - c.x),
                 (m_max.y - c.y),
                 (m_max.z - c.z)};
    }

    const vec4   bbox_min() const { return {m_min.x, m_min.y, m_min.z, m_min.w}; }
    const vec4   bbox_max() const { return {m_max.x, m_max.y, m_max.z, m_max.w}; }

private:
    vec4 m_min = { FLT_MAX, FLT_MAX, FLT_MAX, 1.0f };
    vec4 m_max = { FLT_MIN, FLT_MIN, FLT_MIN, 1.0f };
};


struct plane
{
    plane():nx(.0f), ny(.0f), nz(.0f), d(.0f) {}
    plane(const vec4& p):nx(p.x), ny(p.y), nz(p.z), d(p.w) {}
    plane(float a, float b, float c, float d) :nx(a), ny(b), nz(c), d(d) {}

    MATH_INLINE void  operator = (const vec4& p)                    { set(p.x, p.y, p.z, p.w); }

    MATH_INLINE void  set(float x, float y, float z, float _d)      { nx = x; ny = y; nz = z; d = _d; }

    MATH_INLINE vec3  normal() const                                { return {nx, ny, nz}; }
    MATH_INLINE float distance() const                              { return d; }
    MATH_INLINE float magnitude() const                             { return (float)sqrt(nx * nx + ny * ny + nz * nz); }

    MATH_INLINE plane normalized() const                            { float m = magnitude(); return plane(nx / m, ny / m, nz / m, d / m); }

    MATH_INLINE float  dot(const vec3& v) const                     { return nx * v.x + ny * v.y + nz * v.z + d; }
    MATH_INLINE float  dot(const vec4& v) const                     { return nx * v.x + ny * v.y + nz * v.z + d * v.w; }

    float nx,ny,nz,d;
};


struct frustum
{
    static frustum from_view_proj(const mat4 & viewproj)
    {
        frustum p;
        p.planes[0] = plane(viewproj.column(3) + viewproj.column(0)).normalized(); // (+x) left 
        p.planes[1] = plane(viewproj.column(3) - viewproj.column(0)).normalized(); // (-x) right

        p.planes[2] = plane(viewproj.column(3) + viewproj.column(1)).normalized(); // (+y) bottom
        p.planes[3] = plane(viewproj.column(3) - viewproj.column(1)).normalized(); // (-y) top

        p.planes[4] = plane(viewproj.column(3) + viewproj.column(2)).normalized(); // (+z) near
        p.planes[5] = plane(viewproj.column(3) - viewproj.column(2)).normalized(); // (-z) far
        return p;
    }

    static MATH_INLINE int check_aabbox(const frustum& f, const aabbox& bbox)
    {
        vec3 center = bbox.center();
        vec3 extent = bbox.extends();
        for (int i = 0; i < 6; ++i)
        {
            const plane& p = f.planes[i];

            float d = p.dot(bbox.center());
            float r = extent.x * fabsf(p.nx) + extent.y * fabsf(p.ny) + extent.z * fabsf(p.nz);

            float d_p_r = d + r;
            float d_m_r = d - r + p.d;

            if (d_p_r < 0.0f) return 0; // outside
            if (d_m_r < 0.0f) return 1; // intersect
        }
        return 2; // inside
    }

    static MATH_INLINE int check_point(const frustum& f, const vec3& pos)
    {
        for (int i = 0; i < 6; i++)
            if (f.planes[i].dot(pos) <= 0.0f)
                return 0;
        return 1;
    }

    static MATH_INLINE int check_sphere(const frustum& f, const vec3& pos, float r)
    {
        int c = 0;
        for (int i = 0; i < 6; i++)
        {
            const plane& p = f.planes[i];
            if (p.dot(pos) <= -r)
            {
                return 0;
            }
            c++;
        }
        return (c == 6) ? 1 : 2;
    }

    static MATH_INLINE int check_aabbox_fast(const frustum& f, const aabbox& bbox, const mat4& transform)
    {
        auto bmin = bbox.bbox_min();
        auto bmax = bbox.bbox_max();

        vec3 points[] = {
            math::make_vec3(bmin.x, bmax.y, bmax.z),
            math::make_vec3(bmax.x, bmax.y, bmax.z),
            math::make_vec3(bmin.x, bmin.y, bmax.z),
            math::make_vec3(bmax.x, bmin.y, bmax.z),
            math::make_vec3(bmin.x, bmax.y, bmin.z),
            math::make_vec3(bmax.x, bmax.y, bmin.z),
            math::make_vec3(bmin.x, bmin.y, bmin.z),
            math::make_vec3(bmax.x, bmin.y, bmin.z),
        };

        for (int i = 0; i < 8; ++i)
            points[i] = mat4::mul(transform, points[i]);

        int total = 0;
        for (int i = 0; i < 6; i++)
        {
            const plane& p = f.planes[i];
            int out = 0;
            out += ((p.dot(points[0]) < 0.0) ? 1 : 0);
            out += ((p.dot(points[1]) < 0.0) ? 1 : 0);
            out += ((p.dot(points[2]) < 0.0) ? 1 : 0);
            out += ((p.dot(points[3]) < 0.0) ? 1 : 0);
            out += ((p.dot(points[4]) < 0.0) ? 1 : 0);
            out += ((p.dot(points[5]) < 0.0) ? 1 : 0);
            out += ((p.dot(points[6]) < 0.0) ? 1 : 0);
            out += ((p.dot(points[7]) < 0.0) ? 1 : 0);
            if (out == 8)
                return false;
            total += out;
        }
        return 1;
    }

    plane planes[6];
};


namespace math
{
    MATH_INLINE quat mul(const quat& a, const quat& b)
    {
        float x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
        float y = a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z;
        float z = a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x;
        float w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;

        return {x, y, z, w};
    }

    MATH_INLINE mat4 mul(const mat4 &a, const mat4& b)
    {
        mat4 ret;
        ret.m[0]  = a.m[0] * b.m[0]  + a.m[4] * b.m[1]  + a.m[8]  * b.m[2]  + a.m[12] * b.m[3];
        ret.m[1]  = a.m[1] * b.m[0]  + a.m[5] * b.m[1]  + a.m[9]  * b.m[2]  + a.m[13] * b.m[3];
        ret.m[2]  = a.m[2] * b.m[0]  + a.m[6] * b.m[1]  + a.m[10] * b.m[2]  + a.m[14] * b.m[3];
        ret.m[3]  = a.m[3] * b.m[0]  + a.m[7] * b.m[1]  + a.m[11] * b.m[2]  + a.m[15] * b.m[3];
        ret.m[4]  = a.m[0] * b.m[4]  + a.m[4] * b.m[5]  + a.m[8]  * b.m[6]  + a.m[12] * b.m[7];
        ret.m[5]  = a.m[1] * b.m[4]  + a.m[5] * b.m[5]  + a.m[9]  * b.m[6]  + a.m[13] * b.m[7];
        ret.m[6]  = a.m[2] * b.m[4]  + a.m[6] * b.m[5]  + a.m[10] * b.m[6]  + a.m[14] * b.m[7];
        ret.m[7]  = a.m[3] * b.m[4]  + a.m[7] * b.m[5]  + a.m[11] * b.m[6]  + a.m[15] * b.m[7];
        ret.m[8]  = a.m[0] * b.m[8]  + a.m[4] * b.m[9]  + a.m[8]  * b.m[10] + a.m[12] * b.m[11];
        ret.m[9]  = a.m[1] * b.m[8]  + a.m[5] * b.m[9]  + a.m[9]  * b.m[10] + a.m[13] * b.m[11];
        ret.m[10] = a.m[2] * b.m[8]  + a.m[6] * b.m[9]  + a.m[10] * b.m[10] + a.m[14] * b.m[11];
        ret.m[11] = a.m[3] * b.m[8]  + a.m[7] * b.m[9]  + a.m[11] * b.m[10] + a.m[15] * b.m[11];
        ret.m[12] = a.m[0] * b.m[12] + a.m[4] * b.m[13] + a.m[8]  * b.m[14] + a.m[12] * b.m[15];
        ret.m[13] = a.m[1] * b.m[12] + a.m[5] * b.m[13] + a.m[9]  * b.m[14] + a.m[13] * b.m[15];
        ret.m[14] = a.m[2] * b.m[12] + a.m[6] * b.m[13] + a.m[10] * b.m[14] + a.m[14] * b.m[15];
        ret.m[15] = a.m[3] * b.m[12] + a.m[7] * b.m[13] + a.m[11] * b.m[14] + a.m[15] * b.m[15];
        return ret;
    }

    MATH_INLINE vec3 mul(const quat &q, const vec3& p)
    {
        quat conjugate = quat(-q.x, -q.y, -q.z, q.w);
        quat tmp = mul(q, quat(p.x, p.y, p.z, 0.0f));
        quat res = mul(tmp, conjugate);
        return { res.x, res.y, res.z };
    }

    MATH_INLINE quat inverse(const quat& q);
    MATH_INLINE mat4 inverse(const mat4& m)     { return mat4::inverse(m); }

    // https://gist.github.com/mattatz/86fff4b32d198d0928d0fa4ff32cf6fa
    // https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
    MATH_INLINE quat matrix_to_quat(const mat4& m)
    {
        float trace = m.m00 + m.m11 + m.m22;
        if (trace > 0.0f) {
            float s = sqrtf(trace + 1.0f) * 2.0f; // S=4*qw 
            return quat((m.m21 - m.m12)/s, (m.m02 - m.m20)/s, (m.m10 - m.m01)/s, 0.25f*s );
        } else if ((m.m00 > m.m11) && (m.m00 > m.m22)) {
            float s = sqrtf(1.0f + m.m00 - m.m11 - m.m22) * 2.0f; // S=4*qx 
            return quat(0.25f * s, (m.m01 + m.m10) / s, (m.m02 + m.m20) / s, (m.m21 - m.m12) / s);
        } else if (m.m11 > m.m22) {
            float s = sqrtf(1.0f + m.m11 - m.m00 - m.m22) * 2.0f; // S=4*qy
            return quat((m.m01 + m.m10) / s, 0.25f * s, (m.m12 + m.m21) / s, (m.m02 - m.m20) / s);
        } else {
            float s = sqrtf(1.0f + m.m22 - m.m00 - m.m11) * 2.0f; // S=4*qz
            return quat((m.m02 + m.m20) / s,(m.m12 + m.m21) / s,0.25f * s,(m.m10 - m.m01) / s);
        }
    }

    MATH_INLINE mat4 quat_to_matrix(const quat& quat)
    {
        float x = quat.x, y = quat.y, z = quat.z, w = quat.w;
        float x2 = x + x, y2 = y + y, z2 = z + z;
        float xx = x * x2, xy = x * y2, xz = x * z2;
        float yy = y * y2, yz = y * z2, zz = z * z2;
        float wx = w * x2, wy = w * y2, wz = w * z2;

        float inv_yz = 1.0f - (yy + zz);
        float inv_xz = 1.0f - (xx + zz);
        float inv_xy = 1.0f - (xx + yy);

        return mat4(inv_yz, xy - wz, xz + wy, 0.0f,
                    xy + wz, inv_xz, yz - wx, 0.0f,
                    xz - wy, yz + wx, inv_xy, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
    }

    MATH_INLINE quat look_at_quat(const vec3& dir, const vec3& up)
    {
        vec3 z = normalize(dir);
        vec3 x = normalize(cross(up, z));
        vec3 y = normalize(cross(z, x));

        mat4 m = mat4(x.x, x.y, x.z, 0.0f,
                      y.x, y.y, y.z, 0.0f,
                      z.x, z.y, z.z, 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f);

        return matrix_to_quat(m).inverted();
    }

    MATH_INLINE mat4 look_at_matrix(const vec3& eye, const vec3 &at, const vec3& up, bool rh = is_right_hand())
    {
        vec3 dir = rh ? (eye - at) : (at - eye);
        vec3 z = math::normalize(dir);
        vec3 x = math::normalize(math::cross(up, z));
        vec3 y = math::normalize(math::cross(z, x));

        return mat4(x.x, x.y, x.z, -math::dot(eye, x),
                    y.x, y.y, y.z, -math::dot(eye, y),
                    z.x, z.y, z.z, -math::dot(eye, z),
                    0.0f, 0.0f, 0.0f, 1.0f);
    }

    MATH_INLINE mat4 perspective_matrix(float fov, float aspect, float _near, float _far, bool rh = is_right_hand())
    {
        float ys = 1.0f / tanf(fov * 0.5f);
        float xs = ys / aspect;

        float n = -(_near * _far) / (_far - _near);

        float z = rh ? -1.0f : 1.0f;

        float f = rh ?  (_far / (_near - _far)) :
                        (_far / (_far - _near));

        return mat4(xs,  0.0f, 0.0f, 0.0f,     // m[0] = xs;      ret.m[1] = 0.0f;    ret.m[2] = 0.0f;  ret.m[3] = 0.0f;
                    0.0f, ys,  0.0f, 0.0f,     // m[4] = 0.0f;    ret.m[5] = ys;      ret.m[6] = 0.0f;  ret.m[7] = 0.0f;
                    0.0f, 0.0f, f,   n,        // m[8] = 0.0f;    ret.m[9] = 0.0f;    ret.m[10] = f;    ret.m[11] = z;
                    0.0f, 0.0f, z,   1.0f);    // m[12] = 0.0f;   ret.m[13] = 0.0f;   ret.m[14] = n;    ret.m[15] = 0.0f;
    }

    MATH_INLINE quat euler_to_quat(float roll, float pitch, float yaw)
    {
        vec4 s, c;
        vec4 v { math::deg2rad(roll) , math::deg2rad(pitch) , math::deg2rad(yaw), 1.0f };
        math::vsincos(v * 0.5f, &s, &c);

        return quat{s.x * c.y * c.z - c.x * s.y * s.z,
                    c.x * s.y * c.z + s.x * c.y * s.z,
                    c.x * c.y * s.z - s.x * s.y * c.z,
                    c.x * c.y * c.z + s.x * s.y * s.z};
    }

    MATH_INLINE vec3 quat_to_euler(const quat &q)
    {
        const float sqx = q.x * q.x;
        const float sqy = q.y * q.y;
        const float sqz = q.z * q.z;
        const float sqw = q.w * q.w;

        const float unit = sqx + sqy + sqz + sqw;
        const float test = q.x * q.y + q.z * q.w;

        float yaw = 0.0f;
        float pitch = 0.0f;
        float roll = 0.0f;

        // North pole singularity
        if (test > 0.499f * unit)
        {
            yaw = 2.0f * atan2f(q.x, q.w);
            pitch = math::Pi_half;
            roll = 0.0f;
        }
        // South pole singularity
        else if (test < -0.499f * unit)
        {
            yaw = -2.0f * atan2f(q.x, q.w);
            pitch = -math::Pi_half;
            roll = 0.0f;
        }
        else
        {
            yaw = atan2f(2.0f * q.y * q.w - 2.0f * q.x * q.z, sqx - sqy - sqz + sqw);
            pitch = asinf(2.0f * test / unit);
            roll = atan2f(2.0f * q.x * q.w - 2.0f * q.y * q.z, -sqx + sqy - sqz + sqw);
        }

        // Keep angles [0..360].
        if (yaw < 0.f)
            yaw = math::deg2rad(360.f) + yaw;
        if (pitch < 0.f)
            pitch = math::deg2rad(360.f) + pitch;
        if (roll < 0.f)
            roll = math::deg2rad(360.f) + roll;

        return { math::rad2deg(roll), math::rad2deg(yaw), math::rad2deg(pitch) };
    }

    //mat3f::packTangentFrame({t, b, n});
    MATH_INLINE quat pack_tbn(vec3 n, vec3 t)
    {
        vec3 b = math::cross(n, t);
        return quat::identity();
    //    TMat33<T>{ m[0], cross(m[2], m[0]), m[2] }
    }

}



/*
const tangent = [1, 0, 0, 1];
const normal = [0, 1, 0];

const q = [];
//pack tangent and normal to a quaternion
packTangentFrame(q, normal, tangent);
const n = [], t = [];
//unpack a given quaternion to a normal and tangent.
unpackQuaternion(q, n, t);


void toTangentFrame(const vec4 & q, vec3 &n) {
    n = vec3(0.0, 0.0, 1.0) +
        vec3(2.0, -2.0, -2.0) * q.x * q.zwx +
        vec3(2.0, 2.0, -2.0) * q.y * q.wzy;
}
// 
// Extracts the normal and tangent vectors of the tangent frame encoded in the specified quaternion.

void toTangentFrame(const vec4 &q, out highp vec3 n, out highp vec3 t) {
    toTangentFrame(q, n);
    t = vec3(1.0, 0.0, 0.0) +
        vec3(-2.0, 2.0, -2.0) * q.y * q.yxw +
        vec3(-2.0, 2.0, 2.0) * q.z * q.zwx;
}*/

#endif  // __MATHLIB_H__

 