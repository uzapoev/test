#ifndef __MATHLIB_INL__
#define __MATHLIB_INL__

namespace math
{
    MATH_INLINE float remap(float a1, float a2, float b1, float b2, float s)
    {
        return b1 + (s - a1) * (b2 - b1) / (a2 - a1);
    }


    MATH_INLINE float rsqrt(float x) throw()
    {
        float l = sqrtf(x);
        return (l < Epsilon) ? 1.0f : 1.0f / l;
    }
    MATH_INLINE float qsqrt(float x) throw()
    {
        long i;
        float x2, y;
        const float threehalfs = 1.5F;

        x2 = x * 0.5F;
        y = x;
        i = *(long *)&y;                       // evil floating point bit level hacking
        i = 0x5f3759df - (i >> 1);               // what the fuck? 
        y = *(float *)&i;
        y = y * (threehalfs - (x2 * y * y));   // 1st iteration
        y = y * (threehalfs - (x2 * y * y));   // 2nd iteration, this can be removed
        return 1.0f / y;
    }

    MATH_INLINE float qrsqrt(float number) throw()
    {
        long i;
        float x2, y;
        const float threehalfs = 1.5F;

        x2 = number * 0.5F;
        y = number;
        i = *(long *)&y;                       // evil floating point bit level hacking
        i = 0x5f3759df - (i >> 1);               // what the fuck? 
        y = *(float *)&i;
        y = y * (threehalfs - (x2 * y * y));   // 1st iteration
        y = y * (threehalfs - (x2 * y * y));   // 2nd iteration, this can be removed

        //y = 1 / y;
        return y;
    }

    MATH_INLINE void sincons(const vec4 & v, vec4 * s, vec4 * c)
    {
        //sincos_ps(x, &s, &c);
        s->x = sinf(v.x);
        s->y = sinf(v.y);
        s->z = sinf(v.z);
        s->w = sinf(v.w);

        c->x = cosf(v.x);
        c->y = cosf(v.y);
        c->z = cosf(v.z);
        c->w = cosf(v.w);
    }


    MATH_INLINE float rrandom(float lo, float hi)
    {
        int r = rand();
        float    x = (float)(r & 0x7fff) / (float)0x7fff;
        return (x * (hi - lo) + lo);
    }

    MATH_INLINE float frandom()
    {
        return ((float)rand() / (float)0x3fffffff) - 1.0f;
    }


    namespace ease
    {
        MATH_INLINE float cubic_inout(float T)
        {
            if (T < 0.5f)
                return 4.0f * T * T * T;
            float F = 2.0f * T - 2.0f;
            return (float)(0.5f * F * F * F + 1.0f);
        }

        MATH_INLINE float quart_inout(float T)
        {
            if (T < 0.5f)
            {
                return 8.0f * T * T * T * T;
            }
            else
            {
                float F = T - 1.0f;
                return -8.0f * F * F * F * F + 1.0f;
            }
        }

        // Quintic  
        MATH_INLINE float quint_inout(float T)
        {
            if (T < 0.5f)
            {
                return 16.0f * T * T * T * T * T;
            }
            else
            {
                float F = 2.0f * T - 2.0f;
                return (float)(0.5f * F * F * F * F * F + 1.0f);
            }
        }

        // Exponential
        MATH_INLINE float expo_inout(float T)
        {
            if (T == 0 || T == 1) return T;

            if (T < 0.5f)
            {
                return (float)(0.5f * pow(2.0f, (20.0f * T) - 10.0f));
            }
            else
            {
                return (float)(-0.5f * pow(2.0f, (-20.0f * T) + 10.0f) + 1.0f);
            }
        }

        // Circular
        MATH_INLINE float circ_inout(float T)
        {
            if (T < 0.5f)
            {
                return (float)(0.5f * (1.0f - sqrt(1.0f - 4.0f * T * T)));
            }
            else
            {
                return (float)(0.5f * (sqrt(0.0f - (2.0f * T - 3.0f) * (2.0f * T - 1.0f)) + 1.0f));
            }
        }

        // Back
        MATH_INLINE float back_inout(float T)
        {
            if (T < 0.5f)
            {
                float F = 2.0f * T;
                return 0.5f * (F * F * F - F * sinf(F * Pi));
            }
            else
            {
                float F = 1.0f - (2.0f * T - 1.0f);
                return 0.5f * (1.0f - (F * F * F - F * sinf(F * Pi))) + 0.5f;
            }
        }

        // Elastic
        MATH_INLINE float elastic_inout(float T)
        {
            if (T < 0.5f)
            {
                return (float)(0.5f * sin(13.0f * Pi * T) * pow(2.0f, 10.0f * (2.0f * T - 1.0f)));
            }
            else
            {
                return (float)(0.5f * (sin(-13.0f * Pi * T) * pow(2.0f, -10.0f * (2.0f * T - 1.0f)) + 2.0f));
            }
        }
    };// namespace ease
}// namespace math



/*****************************************************************************/
/*                                                                           */
/* mat4                                                                      */
/*                                                                           */
/*****************************************************************************/
MATH_INLINE mat4::mat4() 
{
    identity();
}


MATH_INLINE mat4::mat4(const mat4 &mat) 
{
    memcpy(m, mat.m, sizeof(m));
}    


MATH_INLINE mat4::mat4(float _m00, float _m01, float _m02, float _m03,
            float _m10, float _m11, float _m12, float _m13,
            float _m20, float _m21, float _m22, float _m23,
            float _m30, float _m31, float _m32, float _m33)
{
    m00 = _m00; m01 = _m01; m02 = _m02; m03 = _m03;
    m10 = _m10; m11 = _m11; m12 = _m12; m13 = _m13;
    m20 = _m20; m21 = _m21; m22 = _m22; m23 = _m23;
    m30 = _m30; m31 = _m31; m32 = _m32; m33 = _m33;
}    



MATH_INLINE vec3 mat4::operator*(const vec3& v) const
{
    return transform_point(v);
}



MATH_INLINE mat4 mat4::operator * (const mat4& mat) const
{
    mat4 ret;
    ret.m[0]  = m[0] * mat.m[0] +  m[4] * mat.m[1]  + m[8]  * mat.m[2] + m[12] * mat.m[3];
    ret.m[1]  = m[1] * mat.m[0] +  m[5] * mat.m[1]  + m[9]  * mat.m[2] + m[13] * mat.m[3];
    ret.m[2]  = m[2] * mat.m[0] +  m[6] * mat.m[1]  + m[10] * mat.m[2] + m[14] * mat.m[3];
    ret.m[3]  = m[3] * mat.m[0] +  m[7] * mat.m[1]  + m[11] * mat.m[2] + m[15] * mat.m[3];
    ret.m[4]  = m[0] * mat.m[4] +  m[4] * mat.m[5]  + m[8]  * mat.m[6] + m[12] * mat.m[7];
    ret.m[5]  = m[1] * mat.m[4] +  m[5] * mat.m[5]  + m[9]  * mat.m[6] + m[13] * mat.m[7];
    ret.m[6]  = m[2] * mat.m[4] +  m[6] * mat.m[5]  + m[10] * mat.m[6] + m[14] * mat.m[7];
    ret.m[7]  = m[3] * mat.m[4] +  m[7] * mat.m[5]  + m[11] * mat.m[6] + m[15] * mat.m[7];
    ret.m[8]  = m[0] * mat.m[8] +  m[4] * mat.m[9]  + m[8]  * mat.m[10] + m[12] * mat.m[11];
    ret.m[9]  = m[1] * mat.m[8] +  m[5] * mat.m[9]  + m[9]  * mat.m[10] + m[13] * mat.m[11];
    ret.m[10] = m[2] * mat.m[8] +  m[6] * mat.m[9]  + m[10] * mat.m[10] + m[14] * mat.m[11];
    ret.m[11] = m[3] * mat.m[8] +  m[7] * mat.m[9]  + m[11] * mat.m[10] + m[15] * mat.m[11];
    ret.m[12] = m[0] * mat.m[12] + m[4] * mat.m[13] + m[8]  * mat.m[14] + m[12] * mat.m[15];
    ret.m[13] = m[1] * mat.m[12] + m[5] * mat.m[13] + m[9]  * mat.m[14] + m[13] * mat.m[15];
    ret.m[14] = m[2] * mat.m[12] + m[6] * mat.m[13] + m[10] * mat.m[14] + m[14] * mat.m[15];
    ret.m[15] = m[3] * mat.m[12] + m[7] * mat.m[13] + m[11] * mat.m[14] + m[15] * mat.m[15];
    return ret;
}


MATH_INLINE void mat4::zero()    
{
    memset(&m[0], 0, sizeof(m));
}


MATH_INLINE void mat4::identity()
{
    zero();
    m[0] = m[5] = m[10] = m[15] = 1.0;
}


MATH_INLINE bool mat4::inverse()
{
    // Inverse = adjoint / det. (See linear algebra texts.)
    // pre-compute 2x2 dets for last two rows when computing
    // cofactors of first two rows.
    float d12 = (m20 * m31 - m30 * m21);
    float d13 = (m20 * m32 - m30 * m22);
    float d23 = (m21 * m32 - m31 * m22);
    float d24 = (m21 * m33 - m31 * m23);
    float d34 = (m22 * m33 - m32 * m23);
    float d41 = (m23 * m30 - m33 * m20);
    
    float tmp[16];
    
    tmp[0] =  (m11 * d34 - m12 * d24 + m13 * d23);
    tmp[1] = -(m10 * d34 + m12 * d41 + m13 * d13);
    tmp[2] =  (m10 * d24 + m11 * d41 + m13 * d12);
    tmp[3] = -(m10 * d23 - m11 * d13 + m12 * d12);
    
    // Compute determinant as early as possible using these cofactors.
    float det = m00 * tmp[0] + m01 * tmp[1] + m02 * tmp[2] + m03 * tmp[3];
    
    // Run singularity test.
    if( det == 0.0 )
    {
        identity();
        return false;
    }
    else
    {
       float invDet = 1.0f / det;
       
       // Compute rest of inverse.
       tmp[0] *= invDet;
       tmp[1] *= invDet;
       tmp[2] *= invDet;
       tmp[3] *= invDet;
    
       tmp[4] = -(m01 * d34 - m02 * d24 + m03 * d23) * invDet;
       tmp[5] =  (m00 * d34 + m02 * d41 + m03 * d13) * invDet;
       tmp[6] = -(m00 * d24 + m01 * d41 + m03 * d12) * invDet;
       tmp[7] =  (m00 * d23 - m01 * d13 + m02 * d12) * invDet;

       // Pre-compute 2x2 dets for first two rows when computing cofactors 
       // of last two rows.
       d12 = m00 * m11 - m10 * m01;
       d13 = m00 * m12 - m10 * m02;
       d23 = m01 * m12 - m11 * m02;
       d24 = m01 * m13 - m11 * m03;
       d34 = m02 * m13 - m12 * m03;
       d41 = m03 * m10 - m13 * m00;
    
       tmp[8]  =  (m31 * d34 - m32 * d24 + m33 * d23) * invDet;
       tmp[9]  = -(m30 * d34 + m32 * d41 + m33 * d13) * invDet;
       tmp[10] =  (m30 * d24 + m31 * d41 + m33 * d12) * invDet;
       tmp[11] = -(m30 * d23 - m31 * d13 + m32 * d12) * invDet;
    
       tmp[12] = -(m21 * d34 - m22 * d24 + m23 * d23) * invDet;
       tmp[13] =  (m20 * d34 + m22 * d41 + m23 * d13) * invDet;
       tmp[14] = -(m20 * d24 + m21 * d41 + m23 * d12) * invDet;
       tmp[15] =  (m20 * d23 - m21 * d13 + m22 * d12) * invDet;
    
       memcpy( m, tmp, 16*sizeof(float) );
    }
  return true;
}


MATH_INLINE void mat4::transpose()
{
    float ret[16] = {0};
    ret[0] = m[0];  ret[4] = m[1];  ret[8] = m[2];   ret[12] = m[3];
    ret[1] = m[4];  ret[5] = m[5];  ret[9] = m[6];   ret[13] = m[7];
    ret[2] = m[8];  ret[6] = m[9];  ret[10] = m[10]; ret[14] = m[11];
    ret[3] = m[12]; ret[7] = m[13]; ret[11] = m[14]; ret[15] = m[15];
    memcpy( m, ret, 16 * sizeof(float) );
}

MATH_INLINE vec3 mat4::transform_point(const vec3 &v) const
{
    float invw = 1.0f / vec4::dot(vec4(v), column(3));
    float x = vec4::dot(vec4(v), column(0)) * invw;
    float y = vec4::dot(vec4(v), column(1)) * invw;
    float z = vec4::dot(vec4(v), column(2)) * invw;

    return vec3(x, y, z);
}


MATH_INLINE vec4 mat4::transform_point(const vec4 &v) const
{
    float x = vec4::dot(v, column(0));
    float y = vec4::dot(v, column(1));
    float z = vec4::dot(v, column(2));
    float w = vec4::dot(v, column(3));
    return vec4(x, y, z, w);
}


MATH_INLINE void mat4::scale(const vec3 & v)
{
    m[0] = v.x;
    m[5] = v.y;
    m[10] = v.z;
    m[15] = 1.0;
}

MATH_INLINE void mat4::translate(const vec3 &v) 
{
    m[12] = v.x; 
    m[13] = v.y; 
    m[14] = v.z; 
}

MATH_INLINE void mat4::rotate(const vec3 & eulers)
{
    vec4 v(math::deg2rad(eulers.x), math::deg2rad(eulers.y), math::deg2rad(eulers.z), 1.0f);
    vec4 c, s;

    math::sincons(v, &s, &c);

    m[0] = (c.y * c.z); m[4] = (s.x * s.y * c.z - c.x * s.z);   m[8] = (c.x * s.y * c.z + s.x * s.z);
    m[1] = (c.y * s.z); m[5] = (s.x * s.y * s.z + c.x * c.z);   m[9] = (c.x * s.y * s.z - s.x * c.z);
    m[2] = (-s.y);      m[6] = (s.x * c.y);                     m[10] = (c.x * c.y);

    m[15] = 1.0f;
}

// angle - in degrees
MATH_INLINE void mat4::rotate(float angle, const vec3 &axis)
{
    float rad = math::deg2rad(angle);
    float c = cosf(rad);
    float s = sinf(rad);
    vec3 v = axis.normalized();
    float xx = v.x * v.x;
    float yy = v.y * v.y;
    float zz = v.z * v.z;
    float xy = v.x * v.y;
    float yz = v.y * v.z;
    float zx = v.z * v.x;
    float xs = v.x * s;
    float ys = v.y * s;
    float zs = v.z * s;

    m[0] = (1.0f - c) * xx + c;  m[4] = (1.0f - c) * xy - zs; m[8] = (1.0f - c) * zx + ys; m[12] = 0.0f;
    m[1] = (1.0f - c) * xy + zs; m[5] = (1.0f - c) * yy + c;  m[9] = (1.0f - c) * yz - xs; m[13] = 0.0f;
    m[2] = (1.0f - c) * zx - ys; m[6] = (1.0f - c) * yz + xs; m[10] = (1.0f - c) * zz + c; m[14] = 0.0f;
    m[3] = 0.0f; m[7] = 0.0f; m[11] = 0.0f; m[15] = 1.0f;
}


void mat4::from_quat(const quat & q) // convert quaternion rotation to matrix, zeros out the translation component.
{
    float xx = q.x*q.x;
    float yy = q.y*q.y;
    float zz = q.z*q.z;
    float xy = q.x*q.y;
    float xz = q.x*q.z;
    float yz = q.y*q.z;
    float wx = q.w*q.x;
    float wy = q.w*q.y;
    float wz = q.w*q.z;

    m[0*4+0] = 1.0f - 2.0f * ( yy + zz );
    m[1*4+0] =        2.0f * ( xy - wz );
    m[2*4+0] =        2.0f * ( xz + wy );

    m[0*4+1] =        2.0f * ( xy + wz );
    m[1*4+1] = 1.0f - 2.0f * ( xx + zz );
    m[2*4+1] =        2.0f * ( yz - wx );

    m[0*4+2] =        2.0f * ( xz - wy );
    m[1*4+2] =        2.0f * ( yz + wx );
    m[2*4+2] = 1.0f - 2.0f * ( xx + yy );

    m[3*4+0] = m[3*4+1] = m[3*4+2] = 0.0f;
    m[0*4+3] = m[1*4+3] = m[2*4+3] = 0.0f;
    m[3*4+3] = 1.0f;
}


void mat4::decompose(vec3 & p, quat &r, vec3 &s)
{
    p.x = m[12];
    p.y = m[13];
    p.z = m[14];

    float det =
        m[0] * (m[5] * m[10] - m[6] * m[9]) -
        m[1] * (m[4] * m[10] - m[6] * m[8]) +
        m[2] * (m[4] * m[9]  - m[5] * m[8]);

    s.x = (det < 0 ? -1 : +1) * sqrtf(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
    s.y = sqrtf(m[4] * m[4] + m[5] * m[5] + m[6] * m[6]);
    s.z = sqrtf(m[8] * m[8] + m[9] * m[9] + m[10] * m[10]);

    r = quat::FromMat(*this);
}

#endif

