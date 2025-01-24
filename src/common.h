#ifndef __common_h__
#define __common_h__

#include <assert.h>
#include <string.h> // memcmp
#include <stdlib.h> // rand,clock
#include <stdint.h> // types
#include <time.h>   // time

#include <string>
#include <chrono>
#include <unordered_set>


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


static void soft_breakpoint()
{
#ifdef _WIN32 
    __debugbreak();
#else
    __builtin_trap();
#endif
}


class bin2hex
{
public:
    static std::string dump(const char* data, size_t len, const char* name);
};


struct Hash
{
    static size_t   murmur32(const void* data, size_t size, unsigned int seed = 5381);
    static int64_t  murmur64(const void *data, size_t size, int64_t seed = 5381);

    static size_t   bernstein_ci(const void* data, size_t size, unsigned int seed = 5381);
};


struct Utf8
{   
    static size_t wchar_to_utf8(const wchar_t* data, size_t size, uint8_t* s);
    static size_t utf8_to_wchar(const uint8_t* data, size_t size, wchar_t* w);
};


struct atomic_string
{
public:
    atomic_string()                                 {}
    atomic_string(const char * str)                 { m_str = make_atom(str); }
    atomic_string(const std::string & str)          { m_str = make_atom(str); }
    atomic_string(const std::string_view & str)     { m_str = make_atom(str.data()); }

    inline size_t          lenght() const           { return m_str.length(); }
    inline const char *    data()   const           { return m_str.data(); }
    inline const char *    c_str()  const           { return m_str.data(); }
    inline bool            empty()  const           { return m_str.length() == 0; }

    inline void operator = (const std::string & str){ m_str = make_atom(str); }
    inline void operator = (const std::string_view & str){ m_str = make_atom(str.data()); }

    inline friend bool operator == (const atomic_string& b1, const atomic_string& b2) { return b1.m_str == b2.m_str; }
  //  inline friend bool operator == (const atomic_string& b1, const atomic_string& b2) { return b1.m_str == b2.m_str; }
    inline friend bool operator <  (const atomic_string& b1, const atomic_string& b2) { return b1.m_str < b2.m_str; }

    static size_t size()
    {
        size_t s = 0;
        for (auto it = interned.begin(); it != interned.end(); ++it)
            s += it->size();
        return s;
    }

private:
    static std::unordered_set <std::string> interned;
    static const char* make_atom(const std::string& value)
    {
        return interned.insert(value).first->c_str();
    }

    std::string_view  m_str;
};

template <> struct std::hash<atomic_string> { 
    inline std::size_t operator() (const atomic_string& s) const 
    { 
        return std::hash<const char*> {} (s.c_str());
    } 
};


class Guid
{
public:
    Guid(void);
    Guid(const char * uuid);
    Guid(const Guid & uuid);
public:
    static Guid             generate_from_seed(size_t seed);
    static void             generate(char *buff, size_t size);
    static bool             validate(const char *buff);
public:
    void                    set(const char * uuid);
    inline const char   *   c_str(void) const           { return m_uuid; }
    inline size_t           size() const                { return sizeof(m_uuid); }

    inline bool             operator <  (const Guid & other) const { return memcmp(c_str(), other.c_str(), size()) < 0;  }
    inline bool             operator == (const Guid & other) const { return memcmp(c_str(), other.c_str(), size()) == 0; }
    inline bool             operator != (const Guid & other) const { return memcmp(c_str(), other.c_str(), size()) != 0; }
protected:
    union
    {
        struct { uint64_t lo, hi; };
        char     m_uuid[32 + 1];// 32 sign + '\0'
    };
};


class measure
{
public:
    measure(const char * msg)
    {
        intend++;
        m_msg = msg;
        m_start = std::chrono::high_resolution_clock::now();
    }
    ~measure()
    {
        intend--;
        m_end = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(m_end - m_start);

        printf(R"("%*s%s: %lldms)", intend*4, " ", m_msg, (long long)diff.count());
      //  printf(R"(%s: %lldms)", m_msg, (long long)diff.count());
    }

private:
    const char * m_msg = "";
    static int intend;
    std::chrono::high_resolution_clock::time_point m_start;
    std::chrono::high_resolution_clock::time_point m_end;
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
