#include <math.h>
#include <memory.h>

#include "mathlib.h"

namespace math
{
    namespace quant
    {
        // [src]: https://gist.github.com/rygorous/2156668
        union FP32
        {
            uint32_t u;
            float f;
            struct
            {
                uint32_t Mantissa : 23;
                uint32_t Exponent : 8;
                uint32_t Sign : 1;
            };
        };

        union FP16
        {
            uint16_t u;
            struct
            {
                uint32_t Mantissa : 10;
                uint32_t Exponent : 5;
                uint32_t Sign : 1;
            };
        };

        static uint16_t encode16_half(float fl)
        {
            FP16 o = { 0 };
            FP32 f; f.f = fl;
            // Based on ISPC reference code (with minor modifications)
            if (f.Exponent == 0) {
                // if Signed zero/denormal (which will underflow)
                o.Exponent = 0;
            }
            else if (f.Exponent == 255) {
                // if Inf or NaN (all exponent bits set)
                o.Exponent = 31;
                o.Mantissa = f.Mantissa ? 0x200 : 0; // NaN->qNaN and Inf->Inf
            }
            else {
                // if Normalized number
                // Exponent unbias the single, then bias the halfp
                int newexp = f.Exponent - 127 + 15;
                if (newexp >= 31) {
                    // if Overflow, return signed infinity
                    o.Exponent = 31;
                }
                else if (newexp <= 0) {
                    // if Underflow
                    if ((14 - newexp) <= 24) {
                        // Mantissa might be non-zero
                        uint32_t mant = f.Mantissa | 0x800000; // Hidden 1 bit
                        o.Mantissa = mant >> (14 - newexp);
                        // Check for rounding
                        if ((mant >> (13 - newexp)) & 1) {
                            // Round, might overflow into exp bit, but this is OK
                            o.u++;
                        }
                    }
                }
                else {
                    o.Exponent = newexp;
                    o.Mantissa = f.Mantissa >> 13;
                    // Check for rounding
                    if (f.Mantissa & 0x1000) {
                        // Round, might overflow to inf, this is OK
                        o.u++;
                    }
                }
            }
            o.Sign = f.Sign;
            return o.u;
        }

        static float decode16_half(uint16_t half)
        {
            static const FP32 magic = { 113 << 23 };
            static const uint32_t shifted_exp = 0x7c00 << 13;
            FP32 o;
            FP16 h; h.u = half;

            o.u = (h.u & 0x7fff) << 13;         // exponent/mantissa bits
            uint32_t exp = shifted_exp & o.u;   // just the exponent

            o.u += (127 - 15) << 23; // exponent adjust
            // handle exponent special cases
            if (exp == shifted_exp) {
                // if Inf/NaN
                // extra exp adjust
                o.u += (128 - 16) << 23;
            }
            else if (exp == 0) {
                // if Zero/Denormal
                // extra exp adjust
                o.u += 1 << 23;
                // renormalize
                o.f -= magic.f;
            }
            // sign bit
            o.u |= (h.u & 0x8000) << 16;
            return o.f;
        }
        
        template<unsigned N> static uint16_t  encode_half(float fl)     { return encode16_half(fl) >> (16 - N); }
        template<unsigned N> static float     decode_half(uint16_t fl)  { return decode16_half(fl << (16 - N)); }
      /*  template<unsigned N> static uint16_t  encode_unorm(float    x)  { return uint16_t(int(x * ((1 << (N)) - 1) + 0.5f)); }
        template<unsigned N> static float     decode_unorm(uint16_t x)  { return x / float((1 << (N)) - 1); }
        template<unsigned N> static uint16_t  encode_snorm(float    x)  { return (x < 0) | (encode_unorm<N - 1>(x < 0 ? -x : x) << 1); }
        template<unsigned N> static float     decode_snorm(uint16_t x)  { return decode_unorm<N - 1>(x >> 1) * (x & 1 ? -1 : 1); }
        */
        //
        // Quantize rotation as 10+10+10 bits for quaternion components
        static void encode101010_quat(uint32_t& out, float x, float y, float z, float w)
        {
            // [ref] http://bitsquid.blogspot.com.es/2009/11/bitsquid-low-level-animation-system.html 
            // "For quaternions we use 2 bits to store the index of the largest component,
            // then 10 bits each to store the value of the remaining three components. We use the knowledge
            // that 1 = x^2 + y^2 + z^2 + w^2 to restore the largest component, so we don't actually have to store its value.
            // Since we don't store the largest component we know that the remaining ones must be in the range (-1/sqrt(2), 1/sqrt(2))
            // (otherwise, one of them would be largest). So we use the 10 bits to quantize a value in that range, giving us a precision of 0.0014.
            // The quaternions (x, y, z, w) and (-x, -y, -z, -w) represent the same rotation, so I flip the signs so that the largest component
            // is always positive." - Niklas Frykholm / bitsquid.se
            static const float rmin = -rsqrt(2), rmax = rsqrt(2);
            float xx = x * x, yy = y * y, zz = z * z, ww = w * w;
            /**/ if (xx >= yy && xx >= zz && xx >= ww) {
                y = remap(y, rmin, rmax, -1, 1);
                z = remap(z, rmin, rmax, -1, 1);
                w = remap(w, rmin, rmax, -1, 1);
                out = x >= 0 ? uint32_t((0 << 30) | (encode_snorm<10>(y) << 20)  | (encode_snorm<10>(z) << 10)  | (encode_snorm<10>(w) << 0))
                             : uint32_t((0 << 30) | (encode_snorm<10>(-y) << 20) | (encode_snorm<10>(-z) << 10) | (encode_snorm<10>(-w) << 0));
            }
            else if (yy >= zz && yy >= ww) {
                x = remap(x, rmin, rmax, -1, 1);
                z = remap(z, rmin, rmax, -1, 1);
                w = remap(w, rmin, rmax, -1, 1);
                out = y >= 0 ? uint32_t((1 << 30) | (encode_snorm<10>(x) << 20)  | (encode_snorm<10>(z) << 10)  | (encode_snorm<10>(w) << 0))
                             : uint32_t((1 << 30) | (encode_snorm<10>(-x) << 20) | (encode_snorm<10>(-z) << 10) | (encode_snorm<10>(-w) << 0));
            }
            else if (zz >= ww) {
                x = remap(x, rmin, rmax, -1, 1);
                y = remap(y, rmin, rmax, -1, 1);
                w = remap(w, rmin, rmax, -1, 1);
                out = z >= 0 ? uint32_t((2 << 30) | (encode_snorm<10>(x) << 20) | (encode_snorm<10>(y) << 10) | (encode_snorm<10>(w) << 0))
                             : uint32_t((2 << 30) | (encode_snorm<10>(-x) << 20) | (encode_snorm<10>(-y) << 10) | (encode_snorm<10>(-w) << 0));
            }
            else {
                x = remap(x, rmin, rmax, -1, 1);
                y = remap(y, rmin, rmax, -1, 1);
                z = remap(z, rmin, rmax, -1, 1);
                out = w >= 0 ? uint32_t((3 << 30) | (encode_snorm<10>(x) << 20)  | (encode_snorm<10>(y) << 10)  | (encode_snorm<10>(z) << 0))
                             : uint32_t((3 << 30) | (encode_snorm<10>(-x) << 20) | (encode_snorm<10>(-y) << 10) | (encode_snorm<10>(-z) << 0));
            }
        }

        //
        // DeQuantize rotation as 10+10+10 bits for quaternion components

        static void decode101010_quat(float& x, float& y, float& z, float& w, uint32_t in)
        {
            // [ref] http://bitsquid.blogspot.com.es/2009/11/bitsquid-low-level-animation-system.html 
            // See encode101010_quat() function above.
            static const float rmin = -rsqrt(2), rmax = rsqrt(2);
            switch (in >> 30) {
            default: case 0:
                y = decode_snorm<10>((in >> 20) & 0x3FF);
                z = decode_snorm<10>((in >> 10) & 0x3FF);
                w = decode_snorm<10>((in >> 0) & 0x3FF);
                y = remap(y, -1, 1, rmin, rmax);
                z = remap(z, -1, 1, rmin, rmax);
                w = remap(w, -1, 1, rmin, rmax);
                x = 1 / rsqrt(1 - y * y - z * z - w * w);
            break; case 1:
                x = decode_snorm<10>((in >> 20) & 0x3FF);
                z = decode_snorm<10>((in >> 10) & 0x3FF);
                w = decode_snorm<10>((in >> 0) & 0x3FF);
                x = remap(x, -1, 1, rmin, rmax);
                z = remap(z, -1, 1, rmin, rmax);
                w = remap(w, -1, 1, rmin, rmax);
                y = 1 / rsqrt(1 - x * x - z * z - w * w);
            break; case 2:
                x = decode_snorm<10>((in >> 20) & 0x3FF);
                y = decode_snorm<10>((in >> 10) & 0x3FF);
                w = decode_snorm<10>((in >> 0) & 0x3FF);
                x = remap(x, -1, 1, rmin, rmax);
                y = remap(y, -1, 1, rmin, rmax);
                w = remap(w, -1, 1, rmin, rmax);
                z = 1 / rsqrt(1 - x * x - y * y - w * w);
            break; case 3:
                x = decode_snorm<10>((in >> 20) & 0x3FF);
                y = decode_snorm<10>((in >> 10) & 0x3FF);
                z = decode_snorm<10>((in >> 0) & 0x3FF);
                x = remap(x, -1, 1, rmin, rmax);
                y = remap(y, -1, 1, rmin, rmax);
                z = remap(z, -1, 1, rmin, rmax);
                w = 1 / rsqrt(1 - x * x - y * y - z * z);
            }
        }

        //
        // Quantize scale/position as X+Y bits for unit vector rotation + Z bits for vector length (note: 2 bits reserved)
        // Requires: each X, Y, Z in [7..16] range && X+Y+Z <= 30

        template<unsigned X, unsigned Y, unsigned Z>
        static void encode_vec(uint32_t& out, float x, float y, float z)
        {
            // Somehow similar to encode101010_quat() function above.
            // We decompose given vector into unit vector and length (magnitude). Then we discard the largest component, as unit vectors
            // follow 1 = x^2 + y^2 + z^2 expression (similar to encode101010_quat() function above). The unit vectors (x, y, z) and
            // (-x, -y, -z) represent the same direction, so I flip the magnitude so that the largest component is always positive.
            static const float rmin = -rsqrt(2), rmax = rsqrt(2);
            float xx = x * x, yy = y * y, zz = z * z;
            float len = rsqrt(xx + yy + zz); // float len = sqrt( xx + yy + zz );
            x *= len; y *= len; z *= len;      // x /= len; y /= len; z /= len;
            /****/ if (xx >= yy && xx >= zz) {
                // y = remap( y, rmin, rmax, -1, 1 );
                // z = remap( z, rmin, rmax, -1, 1 );
                out = (x >= 0 ? uint32_t((0 << 30) | (encode_snorm<X>(y) << (Z + Y)) | (encode_snorm<Y>(z) << Z) | (encode_half<Z>(1 / len))) :  // ( len)
                                uint32_t((0 << 30) | (encode_snorm<X>(-y) << (Z + Y)) | (encode_snorm<Y>(-z) << Z) | (encode_half<Z>(-1 / len)))); // (-len)
            }
            else if (yy >= zz) {
                // x = remap( x, rmin, rmax, -1, 1 );
                // z = remap( z, rmin, rmax, -1, 1 );
                out = (y >= 0 ? uint32_t((1 << 30) | (encode_snorm<X>(x) << (Z + Y)) | (encode_snorm<Y>(z) << Z) | (encode_half<Z>(1 / len))) :  // ( len)
                                uint32_t((1 << 30) | (encode_snorm<X>(-x) << (Z + Y)) | (encode_snorm<Y>(-z) << Z) | (encode_half<Z>(-1 / len)))); // (-len)
            }
            else {
                // x = remap( x, rmin, rmax, -1, 1 );
                // y = remap( y, rmin, rmax, -1, 1 );
                out = (z >= 0 ? uint32_t((2 << 30) | (encode_snorm<X>(x) << (Z + Y)) | (encode_snorm<Y>(y) << Z) | (encode_half<Z>(1 / len))) :  // ( len)
                                uint32_t((2 << 30) | (encode_snorm<X>(-x) << (Z + Y)) | (encode_snorm<Y>(-y) << Z) | (encode_half<Z>(-1 / len)))); // (-len)
            }
        }

        // DeQuantize scale/position as X+Y bits for unit vector rotation + Z bits for vector length (note: 2 bits reserved)
        // Requires: each X, Y, Z in [7..16] range && X+Y+Z <= 30
        template<unsigned X, unsigned Y, unsigned Z>
        static void decode_vec(float& x, float& y, float& z, uint32_t in) 
        {
            // See encode_vec() function above.
            static const float rmin = -rsqrt(2), rmax = rsqrt(2);
            switch (in >> 30) {
            default: case 0:
                y = decode_snorm<X>((in >> (Z + Y)) & ((1 << X) - 1));
                z = decode_snorm<Y>((in >> Z) & ((1 << Y) - 1));
                // y = remap( y, -1, 1, rmin, rmax );
                // z = remap( z, -1, 1, rmin, rmax );
                x = 1 / rsqrt(1 - y * y - z * z); // x = sqrt( 1 - y * y - z * z );
            break; case 1:
                x = decode_snorm<X>((in >> (Z + Y)) & ((1 << X) - 1));
                z = decode_snorm<Y>((in >> Z) & ((1 << Y) - 1));
                // x = remap( x, -1, 1, rmin, rmax );
                // z = remap( z, -1, 1, rmin, rmax );
                y = 1 / rsqrt(1 - x * x - z * z); // y = sqrt( 1 - x * x - z * z );
            break; case 2:
                x = decode_snorm<X>((in >> (Z + Y)) & ((1 << X) - 1));
                y = decode_snorm<Y>((in >> Z) & ((1 << Y) - 1));
                // x = remap( x, -1, 1, rmin, rmax );
                // y = remap( y, -1, 1, rmin, rmax );
                z = 1 / rsqrt(1 - x * x - y * y); // z = sqrt( 1 - x * x - y * y );
            }
            float len = decode_half<Z>(uint16_t(in & ((1 << Z) - 1)));
            x *= len, y *= len, z *= len;
        }

        static void encode161616_vec(uint64_t& out, float x, float y, float z)
        {
            out = ((uint64_t)encode_half<16>(x)) << 32;
            out |= ((uint64_t)encode_half<16>(y)) << 16;
            out |= ((uint64_t)encode_half<16>(z)) << 0;
        }

        static void decode161616_vec(float& x, float& y, float& z, uint64_t in)
        {
            x = decode_half<16>((in >> 32) & 0xffff);
            y = decode_half<16>((in >> 16) & 0xffff);
            z = decode_half<16>((in >> 0) & 0xffff);
        }

        static void encode8814_vec(uint32_t& out, float x, float y, float z)    
        {
            encode_vec<8, 8, 14>(out, x, y, z);        
        }

        static void decode8814_vec(float& x, float& y, float& z, uint32_t in)   
        { 
            decode_vec<8, 8, 14>(x, y, z, in);        
        }

        static void encode555_vec(uint16_t& out, float x, float y, float z)     
        {
            uint16_t x5 = encode_snorm<5>(x);
            uint16_t y5 = encode_snorm<5>(y);
            uint16_t z5 = encode_snorm<5>(z);
            out = (x5 << 10) | (y5 << 5) | (z5 << 0);
        }

        static void decode555_vec(float& x, float& y, float& z, uint16_t in) 
        {
            x = decode_snorm<5>((in >> 10) & 0x1f);
            y = decode_snorm<5>((in >> 5) & 0x1f);
            z = decode_snorm<5>((in >> 0) & 0x1f);
        }
    };
};

