#ifndef __MATHLIB_H__
#define __MATHLIB_H__

#include <stdlib.h>  // rand
#include <stdint.h>  // int8_t


#define MATH_INLINE inline


struct vec2;
struct vec3;
struct vec4;
struct mat4;
struct quat;

namespace math
{
    #undef min
    #undef max

    static const float Epsilon  = 1e-4f;
    static const float Pi       = 3.14159265f;
    static const float Pi_2     = 6.28318521f;  // pi*2
    static const float Pi_half  = 1.57079633f;  // pi/2

    MATH_INLINE float deg2rad(float d)                                              { return d * (Pi / 180.0f); }
    MATH_INLINE float rad2deg(float d)                                              { return d * (180.0f / Pi); }

    template<class T> MATH_INLINE bool fcmp(T f1, T f2)                             { return fabs(f1 - f2) < math::Epsilon; }

    template<class T> MATH_INLINE T min(const T & a, const T & b)                   { return (a < b) ? (a) : (b); }
    template<class T> MATH_INLINE T max(const T & a, const T & b)                   { return (a > b) ? (a) : (b); }
    template<class T> MATH_INLINE T clamp(const T & x, const T & l, const T & h)    { return min(h, max(x, l)); }
    template<class T> MATH_INLINE T lerp(const T &a, const T &b, float t)           { return a + ((b - a) * t); }


    // remap v from a1 a2 range to b1 b2
    MATH_INLINE float remap(float a1, float a2, float b1, float b2, float v);
    MATH_INLINE float rsqrt(float x) throw();   // 1.0f/sqrt(x);
    MATH_INLINE float qsqrt(float x) throw();   // square root
    MATH_INLINE float qrsqrt(float x) throw();  // inv square root 1/sqrt(x)

    MATH_INLINE void sincons(const vec4 & v, vec4 * s, vec4 * c);


    MATH_INLINE float rrandom(float lo, float hi);
    MATH_INLINE float frandom();

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
        MATH_INLINE float cubic_inout(float T);  // implemented in inl file

        // Quartic
        MATH_INLINE float quart_in(float T)     { return T * T * T * T;   }
        MATH_INLINE float quart_out(float T)    { float F = T - 1.0f; return F * F * F * (1 - T) + 1.0f; }
        MATH_INLINE float quart_inout(float T);  // implemented in inl file

        // Quintic
        MATH_INLINE float quint_in(float T)     { return T * T * T * T * T;        }
        MATH_INLINE float quint_out(float T)    { float F = T - 1.0f; return F * F * F * F * F + 1.0f;    }
        MATH_INLINE float quint_inout(float T);  // implemented in inl file

        MATH_INLINE float expo_in(float T)      { return fcmp(T, 0.0f) ? powf(2.0f, 10.0f * (T - 1.0f)) : T; }
        MATH_INLINE float expo_out(float T)     { return fcmp(T, 1.0f) ? T : 1.0f - powf(2.0f, -10.0f * T); }
        MATH_INLINE float expo_inout(float T);   // implemented in inl file

        // Circular
        MATH_INLINE float circ_in(float T)      { return 1.0f - sqrtf(1.0f - T * T); }
        MATH_INLINE float circ_out(float T)     { return sqrtf((2.0f - T) * T); }
        MATH_INLINE float circ_inout(float T);  // implemented in inl file

        // Back
        MATH_INLINE float back_in(float T)      { return T * T * T - T * sinf(T * Pi); }
        MATH_INLINE float back_out(float T)     { float F = 1.0f - T; return 1.0f - (F * F * F - F * sinf(F * Pi)); }
        MATH_INLINE float back_inout(float T);  // implemented in inl file

        // Elastic
        MATH_INLINE float elastic_in(float T)   { return sinf(13.0f * Pi_half * T) * powf(2.0, 10.0f * (T - 1.0f)); }
        MATH_INLINE float elastic_out(float T)  { return sinf(-13.0f * Pi_half * (T + 1.0f)) * powf(2.0f, -10.0f * T) + 1.0f; }
        MATH_INLINE float elastic_inout(float T);   // implemented in inl file

        // k controls the stretching of the func
        MATH_INLINE float impulse(float x, float k)             { float h = k * x; return h * expf(1.0f - h); }
        MATH_INLINE float exppulse(float x, float k, float n)   { return expf(-k * powf(x, n)); }
    };  // namespace ease
};  // namespace math


namespace simd
{
#ifdef _WIN32
    typedef __declspec(align(16)) vec2 avec2;
    typedef __declspec(align(16)) vec4 avec4;
    typedef __declspec(align(16)) vec4 aquat;
    typedef __declspec(align(16)) mat4 amat4;
#else
    typedef vec2 vec2 __attribute__((aligned(16)));
    typedef vec4 vec4 __attribute__((aligned(16)));
    typedef vec4 quat __attribute__((aligned(16)));
    typedef vec4 mat4 __attribute__((aligned(16)));
#endif
    // void lerp(const avec4 * a, const avec4 * b, float t);
    // void slerp(const aquat * a, const aquat * b, float t);
    // void transform(const amat4 & m, const avec4 * v, size_t count, avec4 * o);
}  // namespace simd


#define DEFAULT_OPERATORS_IMPL(_T, _V)  \
    MATH_INLINE             operator float*()               { return _V; }      \
    MATH_INLINE             operator const float*() const   { return _V; }      \
    MATH_INLINE float &     operator[](int i)               { return _V[i]; }   \
    MATH_INLINE const float operator[](int i)   const       { return _V[i]; }


template<class T, size_t C > struct tuple_ops
{
    MATH_INLINE T & as_T()  const                   { return (*(T*)(this)); }
    MATH_INLINE bool operator==(const T &v) const   { T & a = as_T(); for (size_t i = 0; i < C; ++i) if (!math::fcmp(a[i], v[i])) return false; return true; }

    MATH_INLINE T operator*(float f) const          { T res;  T & s = as_T(); for (int i = 0; i < C; ++i) res[i] = s[i] * f; return res; }
    MATH_INLINE T operator/(float f) const          { T res;  T & s = as_T(); for (int i = 0; i < C; ++i) res[i] = s[i] / f; return res; }
    MATH_INLINE T operator+(const T &v) const       { T res;  T & s = as_T(); for (int i = 0; i < C; ++i) res[i] = s[i] + v[i]; return res; }
    MATH_INLINE T operator-(const T &v) const       { T res;  T & s = as_T(); for (int i = 0; i < C; ++i) res[i] = s[i] - v[i]; return res; }

    MATH_INLINE void operator *= (float f)          { T & res = as_T();  for (int i = 0; i < C; ++i) res[i] *= f; }
    MATH_INLINE void operator /= (float f)          { T & res = as_T();  for (int i = 0; i < C; ++i) res[i] /= f; }
    MATH_INLINE void operator += (const T &v)       { T & res = as_T();  for (int i = 0; i < C; ++i) res[i] += v[i]; }
    MATH_INLINE void operator -= (const T &v)       { T & res = as_T();  for (int i = 0; i < C; ++i) res[i] -= v[i]; }
};


struct vec2 : public tuple_ops<vec2, 2u>
{
    DEFAULT_OPERATORS_IMPL(vec2, v);

public:
    vec2() : x(0.0f), y(0.0f)                       { }
    vec2(float x, float y) : x(x), y(y)             { }
    vec2(const vec2 &v) : x(v.x), y(v.y)            { }

public:
    MATH_INLINE void set(float _x, float _y)        { x = _x; y = _y; }

    MATH_INLINE float length()      const           { return math::qsqrt(magnitudeSq()); }
    MATH_INLINE float magnitudeSq() const           { return (x * x) + (y * y); }
    MATH_INLINE void  normalize()                   { float invMag = 1.0f / length();    (*this) *= invMag; }

public:
    friend vec2 operator * (const vec2 &rhs, float lhs)     { return vec2(lhs * rhs.x, lhs * rhs.y); }
    friend vec2 operator + (const vec2& v1, const vec2& v2) { return vec2(v1.x + v2.x, v1.y + v2.y); }
    friend vec2 operator - (const vec2& v1, const vec2& v2) { return vec2(v1.x - v2.x, v1.y - v2.y); }

    union
    {
        struct { float x, y;};
        float v[2];
    };
};



struct vec3 : public tuple_ops<vec3, 3u>
{
    DEFAULT_OPERATORS_IMPL(vec3, v);
    vec3() : x(0.0f), y(0.0f), z(0.0f)                      { }
    vec3(float x):x(x), y(x), z(x)                          { }
    vec3(float x, float y, float z) : x(x), y(y), z(z)      { }
    vec3(const vec3 &v) : x(v.x), y(v.y), z(v.z)            { }

public:
    static const vec3 Up;
    static const vec3 Left;
    static const vec3 Right;
    static const vec3 Forward;
    static const vec3 Zero;

    static vec3  refract(const vec3 &v, const vec3 &N, float eta){ 
        float nv = dot(N, v);
        float k = 1.0f - eta * eta * (1.0f - nv * nv);
        return k < 0.0f ? Zero : eta * v - (eta * nv + sqrtf(k)) * N;
    }
    static vec3  reflect(const vec3 &v, const vec3 &N)          { return v - 2.0f * dot(N, v) * N; }
    static vec3  project(const vec3 & v, const vec3 & normal)   { return v - (dot(v, normal) / normal.lengthsq()) * normal; }
    static vec3  cross(const vec3 & a, const vec3 & b)          { return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
    static float dot(const vec3 & a, const vec3 & b)            { return a.x * b.x + a.y * b.y + a.z * b.z;}
    static float distance(const vec3 & a, const vec3 & b)       { return (a - b).length(); }
//  static vec3 move(const vec3 & src, const vec3 & dst, float step);

public:
    MATH_INLINE void set(float _x, float _y, float _z)      { x = _x; y = _y; z = _z; }
    MATH_INLINE vec2 xy() const                             {  return vec2(x, y); }

    MATH_INLINE float lengthsq()    const                   { return x * x + y * y + z * z; }
    MATH_INLINE float length()      const                   { return sqrtf(x * x + y * y + z * z); }
    MATH_INLINE void  normalize()                           { float invMag = 1.0f / length();  *this *= invMag; }
    MATH_INLINE vec3  normalized() const                    { float m = length(); return vec3(x / m, y / m, z / m); }
    MATH_INLINE vec3  reflect(const vec3 &n) const           { return reflect(*this, n); }

public:
    friend vec3 operator*(float lhs, const vec3 &rhs)       { return vec3(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z); }
    friend vec3 operator-(const vec3 &v)                    { return vec3(-v.x, -v.y, -v.z); }

    union
    {
        struct { float x, y, z; };
        float v[3];
    };
};



struct vec4 : public tuple_ops<vec4, 4u>
{
    DEFAULT_OPERATORS_IMPL(vec4, v);

public:
    vec4(): x(0.0f), y(0.0f), z(0.0f), w(0.0f)                          { }
    vec4(float x, float y, float z) : x(x), y(y), z(z), w(1.0)          { }
    vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w)   { }
//  vec4(const vec2 &v) : x(v.x), y(v.y), z(0.0f),w(1.0f)               { }
    vec4(const vec3 &v) : x(v.x), y(v.y), z(v.z), w(1.0f)               { }
    vec4(const vec4 &v) : x(v.x), y(v.y), z(v.z), w(v.w)                { }

public:
    static vec4  cross(const vec4 &v1, const vec4 &v2)                  { return vec4(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x); }
    static float dot(const vec4 &v1, const vec4 &v2)                    { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w; }
    static float distance(const vec3 & a, const vec3 & b)               { return (a - b).length(); }

public:
    MATH_INLINE void set(float X, float Y, float Z, float W)            { x = X; y = Y; z = Z; w = W; }
    MATH_INLINE vec3 xyz() const                                        { return vec3(x, y, z); }
    MATH_INLINE vec2 xy() const                                         { return vec2(x, y); }

    MATH_INLINE float lengthsq()    const                               { return x * x + y * y + z * z + w * w; }
    MATH_INLINE float length() const                                    { return sqrtf(x * x + y * y + z * z + w * w); }
    MATH_INLINE void  normalize()                                       { float l = 1.0f/length(); set(x*l, y*l, z*l, w*l); }
    MATH_INLINE vec4  normalized() const                                { float l = length(); return vec3(x / l, y / l, z / l); }

    MATH_INLINE float dot(const vec4 &v)const                           { return x * v.x + y * v.y + z * v.z + w*v.w; }

public:
    friend vec4 operator*(float lhs, const vec4 &rhs)                   { return vec4(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w); }
    friend vec4 operator-(const vec4 &v)                                { return vec4(-v.x, -v.y, -v.z, -v.w); }

    union
    {
        struct { float x, y, z, w; };
        float v[4];
    };
};


//  http://willperone.net/Code/quaternion.php
struct quat
{
    DEFAULT_OPERATORS_IMPL(quat, q)

public:
    quat():x(0.0f), y(0.0f), z(0.0f), w(1.0f)                           {}
    quat(float * v):x(v[0]), y(v[1]), z(v[2]), w(v[3])                  {}
    quat(float x, float y, float z, float w):x(x), y(y), z(z), w(w)     {}

public:
    static quat     FromEulers(float x, float y, float z);
    static quat     FromMat(const mat4 &m);
    static quat     LookAt(const vec3 & dir, const vec3 & up);
    static quat     LookAt(const vec3 & from, const vec3 & to, const vec3 & up);

    static float    dot(const quat &a, const quat &b)                   { return ((a.w * b.w) + (a.x * b.x) + (a.y * b.y) + (a.z * b.z)); }
    static quat     slerp(const quat &a, const quat &b, float t);

public:
    MATH_INLINE  void set(float _x, float _y, float _z, float _w)       { x = _x; y = _y; z = _z; w = _w;}

    MATH_INLINE quat operator * (float scalar) const                    { return quat(x * scalar, y * scalar, z * scalar, w * scalar); }
    MATH_INLINE quat operator / (float scalar) const                    { return quat(x / scalar, y / scalar, z / scalar, w / scalar); }

public:
    MATH_INLINE  void  compute_w()                                      { float t = 1.0f - (x * x) - (y * y) - (z * z); w = (t < 0.0f)?0.0f:-sqrtf(t);}
    MATH_INLINE  void  identity()                                       {  w = 1.0f, x = y = z = 0.0f; }

    MATH_INLINE  float length() const                                   { return sqrtf(x * x + y * y + z * z + w * w); }
    MATH_INLINE  quat  conjugate() const                                { return quat(-x, -y, -z, w); }
    MATH_INLINE  quat  inverted() const                                 { return conjugate() / (x * x + y * y + z * z + w * w); }

    mat4 to_mat4() const;
    vec3 to_euler(bool homoheus = false);

    vec3 rotate_point(const vec3 &v) const;
    vec4 rotate_point(const vec4 &v) const;
    void rotate(float x, float y, float z);
    void rotate(float degrees, const vec3 &axis);

public:
    friend quat operator*(const quat &lhs, const quat &rhs);
    friend vec3 operator*(const quat &rhs, const vec3 &v)               { return rhs.rotate_point(v); }
    friend vec3 operator*(const vec3 &v, const quat &rhs)               { return rhs.rotate_point(v); }
    friend quat operator*(const quat &rhs, float s)                     { return quat(rhs.x * s, rhs.y * s, rhs.z * s, rhs.w *s); }

public:
    union
    {
        struct { float x, y, z, w; };
        float q[4];
    };
};


struct mat4
{
    DEFAULT_OPERATORS_IMPL(mat4, m)
    mat4();
    mat4(const mat4 &mat);
    mat4(float _m00, float _m01, float _m02, float _m03,
         float _m10, float _m11, float _m12, float _m13,
         float _m20, float _m21, float _m22, float _m23,
         float _m30, float _m31, float _m32, float _m33);

public:
    static mat4 Zero();
    static mat4 Identity();
    static mat4 FromQuat(const quat &q);

    static mat4 lookAtLH(const vec3 &eye, const vec3 &at, const vec3 &up);
    static mat4 lookAtRH(const vec3 &eye, const vec3 &at, const vec3 &up);
    static mat4 perspectiveFovLH(float fovY, float aspectRatio, float zNear, float zFar);  // zFar == 0 -> far plane at infinity
    static mat4 perspectiveFovRH(float fovY, float aspectRatio, float zNear, float zFar);  // zFar == 0 -> far plane at infinity
    static mat4 orthoOffCenterLH(float l, float r, float b, float t, float zNear, float zFar);
    static mat4 orthoOffCenterRH(float l, float r, float b, float t, float zNear, float zFar);
    static mat4 orthoLH(float width, float height, float zNear, float zFar);
    static mat4 orthoRH(float width, float height, float zNear, float zFar);

public:
    MATH_INLINE vec3 operator * (const vec3& v) const;
    MATH_INLINE mat4 operator * (const mat4& mat) const;

    MATH_INLINE void zero();
    MATH_INLINE void identity();

    MATH_INLINE bool inverse();
    MATH_INLINE void transpose();

    MATH_INLINE vec4 row(int i) const    { return vec4(m[i * 4 + 0], m[i * 4 + 1], m[i * 4 + 2], m[i * 4 + 3]); }
    MATH_INLINE vec4 column(int i)const  { return vec4(m[i], m[i+4], m[i+8], m[i+12]); }

    MATH_INLINE vec3 transform_point(const vec3 &v) const;
    MATH_INLINE vec4 transform_point(const vec4 &v) const;

    MATH_INLINE void scale(const vec3 & v);
    MATH_INLINE void translate(const vec3 &v);
    MATH_INLINE void rotate(const vec3 & eulers);
    MATH_INLINE void rotate(float angle, const vec3 &axis);   // angle - in degrees

    MATH_INLINE void from_quat(const quat & q);  // convert quaternion rotation to matrix, zeros out the translation component.
    MATH_INLINE void decompose(vec3 & p, quat & r, vec3 & s);

    union
    {
        float m_Mat[4][4];
        float m[16];
        struct {
            float m00, m10, m20, m30;
            float m01, m11, m21, m31;
            float m02, m12, m22, m32;
            float m03, m13, m23, m33;
        };
    };
};


struct color32
{
    static inline const color32 Clear()     { return color32(0, 0, 0, 0); }
    static inline const color32 White()     { return color32(255, 255, 255, 255); }
    static inline const color32 Black()     { return color32(0, 0, 0, 255); }
    static inline const color32 Red()       { return color32(255, 0, 0, 255); }
    static inline const color32 Green()     { return color32(0, 255, 0, 255); }
    static inline const color32 Blue()      { return color32(0, 0, 255, 255); }
    static inline const color32 Magenta()   { return color32(255, 255, 0, 255); }
    
    color32(uint32_t _rgba)
        :rgba(_rgba)
    {}

    color32(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a)
        :r(_r), g(_g), b(_b), a(_a)
    { }

    inline operator uint32_t() const { return rgba; }
//  inline uint16_t to_4444(color32 rgba);
 // inline uint16_t to_5551(color32 rgba);
 // inline uint16_t to_565(color32 rgba);

//  inline uint32_t from_4444(uint16_t);
 // inline uint32_t from_5551(uint16_t);
 // inline uint32_t from_565(uint16_t);

    union
    {
        uint32_t rgba;
        struct
        {
            uint8_t r, g, b, a;
        };
    };
};

#include "mathlib.inl"

#endif  // __MATHLIB_H__

