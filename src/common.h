#ifndef __common_h__
#define __common_h__

#include <assert.h>
#include <string.h> // memcmp
#include <stdlib.h> // rand,clock
#include <stdint.h> // types
#include <time.h>   // time


class Time
{
public:
    static float    dt();       // delta time between ticks
    static float    elapsed();
    static void     tick();     // update timers

private:
    static float    s_dt;
    static float    s_elapsed_time;
};


class Hash
{
public:
    static size_t   murmur32(const void* data, size_t size, unsigned int seed = 5381);
    static int64_t  murmur64(const void *data, size_t size, int64_t seed = 5381);

    static size_t   bernstein_ci(const void* data, size_t size, unsigned int seed = 5381);
};


class Guid
{
public:
    Guid(void);
    Guid(const char * uuid);
    Guid(const Guid & uuid);
public:
    static void             generate(char *buff, size_t size);
    static bool             isguid(const char *buff);
public:
    void                    set(const char * uuid);
    inline const char   *   c_str(void) const           { return m_uuid; }
    inline size_t           size() const                { return sizeof(m_uuid); }

    inline bool             operator <  (const Guid & other) const { return memcmp(c_str(), other.c_str(), size()) < 0; }
    inline bool             operator == (const Guid & other) const { return memcmp(c_str(), other.c_str(), size()) == 0; }
    inline bool             operator != (const Guid & other) const { return memcmp(c_str(), other.c_str(), size()) != 0; }
protected:
    char                    m_uuid[32 + 1];// 32 sign + '\0'
};


// [refs]
// - https://gist.github.com/rygorous/2156668
// - http://zeuxcg.org/2010/12/14/quantizing-floats/
// - http://en.wikipedia.org/wiki/Fast_inverse_square_root
// - http://bitsquid.blogspot.com.es/2009/11/bitsquid-low-level-animation-system.html 
namespace quantinizer
{
    uint16_t    encode16f(float value);
    float       decode16f(uint16_t value);

    void        encode101010_quat(uint32_t &out, float x, float y, float z, float w);
    void        decode101010_quat(float &x, float &y, float &z, float &w, uint32_t in);

    void        encode555_vec(uint16_t &out, float x, float y, float z);   // position or scale to 16-bit integer (struct version)
    void        decode555_vec(float &x, float &y, float &z, uint16_t in);  // 16-bit integer to position or scale (struct version)
}

#endif
