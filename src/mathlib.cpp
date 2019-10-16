#include <math.h>
#include <memory.h>
//#include <string.h>
//#include <memory.h>

#include "mathlib.h"


const vec3 vec3::Up         = vec3( 0.0f, 1.0f, 0.0f);
const vec3 vec3::Left       = vec3( 1.0f, 0.0f, 0.0f);
const vec3 vec3::Right      = vec3(-1.0f, 0.0f, 0.0f);
const vec3 vec3::Forward    = vec3( 0.0f, 0.0f, 1.0f);
const vec3 vec3::Zero       = vec3( 0.0f, 0.0f, 0.0f);



/*****************************************************************************/
/*                                                                           */
/* mat4                                                                      */
/*                                                                           */
/*****************************************************************************/
mat4::mat4()
{
    identity();
}


mat4::mat4(const mat4 &mat)
{
    for (int i = 0; i < 16; ++i)
        m[i] = mat.m[i];
    //memcpy(m, mat.m, sizeof(m));
}


mat4::mat4(float _m00, float _m01, float _m02, float _m03,
    float _m10, float _m11, float _m12, float _m13,
    float _m20, float _m21, float _m22, float _m23,
    float _m30, float _m31, float _m32, float _m33)
{
    m00 = _m00; m01 = _m01; m02 = _m02; m03 = _m03;
    m10 = _m10; m11 = _m11; m12 = _m12; m13 = _m13;
    m20 = _m20; m21 = _m21; m22 = _m22; m23 = _m23;
    m30 = _m30; m31 = _m31; m32 = _m32; m33 = _m33;
}

quat quat::FromEulers(float x, float y, float z)
{
    quat q;
    q.rotate(x,y,z);
    return q;
}


quat quat::FromMat(const mat4 &m)
{
    float s = 0.0f;
    float q[4] = { 0.0f };
    float trace = m.m_Mat[0][0] + m.m_Mat[1][1] + m.m_Mat[2][2];

    if (trace > 0.0f)
    {
        s = sqrtf(trace + 1.0f);
        q[3] = s * 0.5f;
        s = 0.5f / s;
        q[0] = (m.m_Mat[1][2] - m.m_Mat[2][1]) * s;
        q[1] = (m.m_Mat[2][0] - m.m_Mat[0][2]) * s;
        q[2] = (m.m_Mat[0][1] - m.m_Mat[1][0]) * s;
    }
    else
    {
        int nxt[3] = { 1, 2, 0 };
        int i = 0, j = 0, k = 0;

        if (m.m_Mat[1][1] > m.m_Mat[0][0])
            i = 1;

        if (m.m_Mat[2][2] > m.m_Mat[i][i])
            i = 2;

        j = nxt[i];
        k = nxt[j];
        s = sqrtf((m.m_Mat[i][i] - (m.m_Mat[j][j] + m.m_Mat[k][k])) + 1.0f);

        q[i] = s * 0.5f;
        s = 0.5f / s;
        q[3] = (m.m_Mat[j][k] - m.m_Mat[k][j]) * s;
        q[j] = (m.m_Mat[i][j] + m.m_Mat[j][i]) * s;
        q[k] = (m.m_Mat[i][k] + m.m_Mat[k][i]) * s;
    }
    return quat(q[0], q[1], q[2], q[3]);
}


quat quat::LookAt(const vec3& dir, const vec3 & up)
{
    vec3 zAxis = dir.normalized();
    vec3 xAxis = vec3::cross(up, zAxis).normalized();
    vec3 yAxis = vec3::cross(zAxis, xAxis).normalized();

    mat4 m = mat4(  xAxis.x, xAxis.y, xAxis.z, 0.0f,
                    yAxis.x, yAxis.y, yAxis.z, 0.0f,
                    zAxis.x, zAxis.y, zAxis.z, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);

    return quat::FromMat(m).inverted();
    /*
    float s = 0.0f;
    float q[4] = { 0.0f };
    float trace = m.m_Mat[0][0] + m.m_Mat[1][1] + m.m_Mat[2][2];

    if (trace > 0.0f)
    {
        s = sqrtf(trace + 1.0f);
        q[3] = s * 0.5f;
        s = 0.5f / s;
        q[0] = (m.m_Mat[1][2] - m.m_Mat[2][1]) * s;
        q[1] = (m.m_Mat[2][0] - m.m_Mat[0][2]) * s;
        q[2] = (m.m_Mat[0][1] - m.m_Mat[1][0]) * s;
    }
    else
    {
        int nxt[3] = { 1, 2, 0 };
        int i = 0, j = 0, k = 0;

        if (m.m_Mat[1][1] > m.m_Mat[0][0])
            i = 1;

        if (m.m_Mat[2][2] > m.m_Mat[i][i])
            i = 2;

        j = nxt[i];
        k = nxt[j];
        s = sqrtf((m.m_Mat[i][i] - (m.m_Mat[j][j] + m.m_Mat[k][k])) + 1.0f);

        q[i] = s * 0.5f;
        s = 0.5f / s;
        q[3] = (m.m_Mat[j][k] - m.m_Mat[k][j]) * s;
        q[j] = (m.m_Mat[i][j] + m.m_Mat[j][i]) * s;
        q[k] = (m.m_Mat[i][k] + m.m_Mat[k][i]) * s;
    }
    return quat(q[0], q[1], q[2], q[3]).inverted();*/
}


quat quat::LookAt(const vec3& from, const vec3& to, const vec3 & up)
{
    vec3 dir = from - to;
    return quat::LookAt(dir, up);
}


//https://github.com/zeux/zeux.github.io/blob/master/_posts/2015-07-23-approximating-slerp.md
quat quat::slerp(const quat &q0, const quat &q1, float t)
{
    float co = quat::dot(q0, q1);

    float d = fabsf(co);
    float k = d * (d * 0.331442f - 1.25654f) + 0.931872f;
    float ot = t + t * (t - 0.5f) * (t - 1.0f) * k;
    float lt = 1.0f - ot;
    float rt = co > 0.0f ? ot : -ot;

    return quat(
        (lt * q0.x) + (rt * q1.x),
        (lt * q0.y) + (rt * q1.y),
        (lt * q0.z) + (rt * q1.z),
        (lt * q0.w) + (rt * q1.w)
        );
}


quat operator*(const quat &a, const quat &b) 
{
    float x = a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y;
    float y = a.w*b.y + a.y*b.w + a.z*b.x - a.x*b.z;
    float z = a.w*b.z + a.z*b.w + a.x*b.y - a.y*b.x;
    float w = a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z;

    return quat(x, y, z, w);
}


vec3 quat::rotate_point(const vec3 &v) const
{
    quat tmp = quat(*this) * quat(v.x, v.y, v.z, 0.0f);
    quat res = tmp * conjugate();
    return vec3(res.x, res.y, res.z);
}


vec4 quat::rotate_point(const vec4 &v) const
{
    quat tmp = quat(*this) * quat(v.x, v.y, v.z, 0.0f);
    quat res = tmp * conjugate();
    return vec4(res.x, res.y, res.z, 0.0f);
}


void quat::rotate(float _x, float _y, float _z)
{
    vec4 v(math::deg2rad(_x) * 0.5f, 
           math::deg2rad(_y) * 0.5f, 
           math::deg2rad(_z) * 0.5f, 1.0f);
    vec4 s, c;

    math::sincons(v, &s, &c);

    x = s.x * c.y * c.z + c.x * s.y * s.z;
    y = c.x * s.y * c.z - s.x * c.y * s.z;
    z = c.x * c.y * s.z + s.x * s.y * c.z;
    w = c.x * c.y * c.z - s.x * s.y * s.z;
}


void quat::rotate(float degrees, const vec3 &axis)
{
    float halfTheta = math::deg2rad(degrees) * 0.5f;
    float s = sinf(halfTheta);
    w = cosf(halfTheta); 
    x = axis.x * s; 
    y = axis.y * s; 
    z = axis.z * s;
}


// Converts this quaternion to a rotation matrix.
//
//  | 1 - 2(y^2 + z^2)  2(xy + wz)          2(xz - wy)          0  |
//  | 2(xy - wz)        1 - 2(x^2 + z^2)    2(yz + wx)          0  |
//  | 2(xz + wy)        2(yz - wx)          1 - 2(x^2 + y^2)    0  |
//  | 0                 0                   0                   1  |
//
mat4 quat::to_mat4() const
{
    const float sqw = w * w;
    const float sqx = x * x;
    const float sqy = y * y;
    const float sqz = z * z;

    mat4 m = mat4::Identity();
    // invs (inverse square length) is only required if quaternion is not already normalised
    float invs = 1 / (sqx + sqy + sqz + sqw);
    m.m00 = ( sqx - sqy - sqz + sqw) * invs; // since sqw + sqx + sqy + sqz =1/invs*invs
    m.m11 = (-sqx + sqy - sqz + sqw) * invs;
    m.m22 = (-sqx - sqy + sqz + sqw) * invs;

    float tmp1 = x * y;
    float tmp2 = z * w;
    m.m10 = 2.0f * (tmp1 + tmp2) * invs;
    m.m01 = 2.0f * (tmp1 - tmp2) * invs;

    tmp1 = x * z;
    tmp2 = y * w;
    m.m20 = 2.0f * (tmp1 - tmp2) * invs;
    m.m02 = 2.0f * (tmp1 + tmp2) * invs;
    tmp1 = y * z;
    tmp2 = x * w;
    m.m21 = 2.0f * (tmp1 + tmp2) * invs;
    m.m12 = 2.0f * (tmp1 - tmp2) * invs;
    return m;
}


vec3 quat::to_euler(bool homogenous)
{
    const float sqw = w * w;
    const float sqx = x * x;
    const float sqy = y * y;
    const float sqz = z * z;

    const float unit = sqx + sqy + sqz + sqw;
    const float test = x * y + z * w;

    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;

    // North pole singularity
    if (test > 0.499f * unit)
    {
        yaw = 2.0f * atan2f(x, w);
        pitch = math::Pi_half;
        roll = 0.0f;
    }

    // South pole singularity
    else if (test < -0.499f * unit)
    {
        yaw = -2.0f * atan2f(x, w);
        pitch = -math::Pi_half;
        roll = 0.0f;
    }
    else
    {
        yaw = atan2f(2.0f * y * w - 2.0f * x * z, sqx - sqy - sqz + sqw);
        pitch = asinf(2.0f * test / unit);
        roll = atan2f(2.0f * x * w - 2.0f * y * z, -sqx + sqy - sqz + sqw);
    }
    
    // Keep angles [0..360].
    if (yaw < 0.f)
        yaw = math::deg2rad(360.f) + yaw;
    if (pitch < 0.f)
        pitch = math::deg2rad(360.f) + pitch;
    if (roll < 0.f)
        roll = math::deg2rad(360.f) + roll;
    
    return vec3(math::rad2deg(roll), math::rad2deg(yaw), math::rad2deg(pitch));
}


mat4 mat4::Zero()
{
    mat4 m;
    m.zero();
    return m;
}


mat4 mat4::Identity()
{
    return mat4();
}


mat4 mat4::lookAtLH(const vec3 &eye, const vec3 &at, const vec3 &up)
{
    vec3 zAxis = vec3(at - eye).normalized();
    vec3 xAxis = vec3::cross(up, zAxis).normalized();
    vec3 yAxis = vec3::cross(zAxis, xAxis).normalized();

    return mat4(xAxis.x, xAxis.y, xAxis.z, -vec3::dot(eye, xAxis),
                yAxis.x, yAxis.y, yAxis.z, -vec3::dot(eye, yAxis),
                zAxis.x, zAxis.y, zAxis.z, -vec3::dot(eye, zAxis),
                0.0f, 0.0f, 0.0f, 1.0f);
}


mat4 mat4::lookAtRH(const vec3 &eye, const vec3 &at, const vec3 &up)
{
    vec3 zAxis = vec3(eye - at).normalized();
    vec3 xAxis = vec3::cross(up, zAxis).normalized();
    vec3 yAxis = vec3::cross(zAxis , xAxis).normalized();

    return mat4 ( xAxis.x, xAxis.y, xAxis.z, -vec3::dot(eye, xAxis),
                  yAxis.x, yAxis.y, yAxis.z, -vec3::dot(eye, yAxis),
                  zAxis.x, zAxis.y, zAxis.z, -vec3::dot(eye, zAxis),
                  0.0f, 0.0f, 0.0f, 1.0f );
}


mat4 mat4::perspectiveFovLH(float fovY, float aspectRatio, float zNear, float zFar)
{
    mat4 ret;
    float yScale = 1.0f / tanf(fovY / 2.0f);
    float xScale = yScale / aspectRatio;

    ret.m_Mat[0][0] = xScale;   ret.m_Mat[0][1] = 0.0f;     ret.m_Mat[0][2] = 0.0f;                             ret.m_Mat[0][3] = 0.0f;
    ret.m_Mat[1][0] = 0.0f;     ret.m_Mat[1][1] = yScale;   ret.m_Mat[1][2] = 0.0f;                             ret.m_Mat[1][3] = 0.0f;
    ret.m_Mat[2][0] = 0.0f;     ret.m_Mat[2][1] = 0.0f;     ret.m_Mat[2][2] = zFar / (zFar - zNear);            ret.m_Mat[2][3] = 1.0f;
    ret.m_Mat[3][0] = 0.0f;     ret.m_Mat[3][1] = 0.0f;     ret.m_Mat[3][2] = -zNear * zFar / (zFar - zNear);   ret.m_Mat[3][3] = 0.0f;

    if (zFar == 0.0f)
    {
        ret.m_Mat[2][2] = 1.0f;
        ret.m_Mat[3][2] = -zNear;
    }

    return ret;
}


mat4 mat4::perspectiveFovRH(float fovY, float aspectRatio, float zNear, float zFar)
{
    mat4 ret;
    float yScale = 1.0f / tanf(fovY * 0.5f);
    float xScale = yScale / aspectRatio;
    float f = zFar / (zNear - zFar);
    float n = zNear * zFar / (zNear - zFar);

    ret.m[0] = xScale;   ret.m[1] = 0.0f;     ret.m[2] = 0.0f;  ret.m[3] = 0.0f;
    ret.m[4] = 0.0f;     ret.m[5] = yScale;   ret.m[6] = 0.0f;  ret.m[7] = 0.0f;
    ret.m[8] = 0.0f;     ret.m[9] = 0.0f;     ret.m[10] = f;    ret.m[11] = -1.0f;
    ret.m[12] = 0.0f;    ret.m[13] = 0.0f;    ret.m[14] = n;    ret.m[15] = 0.0f;

    if (zFar == 0.0f)
    {
        ret.m[10] = -1.0f;
        ret.m[14] = -zNear;
    }
    return ret;
}


mat4 mat4::orthoOffCenterLH(float l, float r, float b, float t, float zNear, float zFar)
{
    mat4 ret;
    ret.m[0] = 2.0f / (r - l);           ret.m[1] = 0.0f;               ret.m[2] = 0.0f;                       ret.m[3] = 0.0f;
    ret.m[4] = 0.0f;                     ret.m[5] = 2.0f / (t - b);     ret.m[6] = 0.0f;                       ret.m[7] = 0.0f;
    ret.m[8] = 0.0f;                     ret.m[9] = 0.0f;               ret.m[10] = 1.0f / (zFar - zNear);     ret.m[11] = 0.0f;
    ret.m[12] = (1.0f + r) / (1.0f - r); ret.m[13] = (t + b) / (b - t); ret.m[14] = -zNear / (zFar - zNear);   ret.m[15] = 1.0f;
    return ret;
}


mat4 mat4::orthoOffCenterRH(float l, float r, float b, float t, float zNear, float zFar)
{
    mat4 ret;
    ret.m[0] = 2.0f / (r - l);           ret.m[1] = 0.0f;               ret.m[2] = 0.0f;                      ret.m[3] = 0.0f;
    ret.m[4] = 0.0f;                     ret.m[5] = 2.0f / (t - b);     ret.m[6] = 0.0f;                      ret.m[7] = 0.0f;
    ret.m[8] = 0.0f;                     ret.m[9] = 0.0f;               ret.m[10] = 1.0f / (zNear - zFar);    ret.m[11] = 0.0f;
    ret.m[12] = (1.0f + r) / (1.0f - r); ret.m[13] = (t + b) / (b - t); ret.m[14] = -zNear / (zFar - zNear);  ret.m[15] = 1.0f;
    return ret;
}


mat4 mat4::orthoLH(float width, float height, float zNear, float zFar)
{
    mat4 ret;
    return ret;
}


mat4 mat4::orthoRH(float width, float height, float zNear, float zFar)
{
    mat4 ret;
    return ret;
}