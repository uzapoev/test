#ifndef __common_h__
#define __common_h__

#include <assert.h>
#include <string.h> // memcmp
#include <stdlib.h> // rand,clock
#include <stdarg.h> // va_arg
#include <stdint.h> // types
#include <time.h>   // time

#include <string>
#include <chrono>
#include <mutex>
#include <functional>
#include <unordered_set>

//#include "threads.h"

#define PROFILE_SAMPLE(SAMPLE_NAME) measure ms(SAMPLE_NAME);

struct Time
{
    static float    dt();       // delta time between ticks
    static float    elapsed();  // elapsed time sins app started
    static void     tick();     // update timers
};


struct bin2hex
{
    static std::string dump(const char* data, size_t len, const char* name);
};


struct Hash
{
    static uint32_t murmur32(const void* data, uint32_t size, uint32_t seed = 5381);
    static uint64_t murmur64(const void *data, uint32_t size, uint32_t seed = 5381);
    static size_t   bernstein_ci(const void* data, uint32_t size, uint32_t seed = 5381);
};


struct utf8
{
    static bool         is_ascii(const char* data, size_t size);
    static bool         is_ascii(const wchar_t* data, size_t size);

    static size_t       wchar_to_utf8(const wchar_t* data, size_t size, uint8_t* s);
    static size_t       utf8_to_wchar(const uint8_t* data, size_t size, wchar_t* w);

    static std::wstring from_utf8(const uint8_t* data, size_t size);
};


struct debug
{
    static void     log(const char* msg, ...);
    static void     log_error(const char* msg, ...);
    static void     log_warning(const char* msg, ...);

    static void     breakpoint();
    static void     callstack(uintptr_t * frames, uint32_t count);
};


struct interned_string
{
public:
    interned_string()                                       {}
    interned_string(const char * str)                       { m_str = make_intern(str); }
    interned_string(const std::string & str)                { m_str = make_intern(str.data()); }
    interned_string(const std::string_view & str)           { m_str = make_intern(str.data()); }

    void                clear()                             { m_str = ""; }
    inline size_t       length() const                      { return m_str.length(); }
    inline const char * data()   const                      { return m_str.data(); }
    inline const char * c_str()  const                      { return m_str.data(); }
    inline bool         empty()  const                      { return m_str.length() == 0; }

    inline friend bool operator == (const interned_string& b1, const interned_string& b2) { return b1.m_str == b2.m_str; }
    inline friend bool operator <  (const interned_string& b1, const interned_string& b2) { return b1.m_str < b2.m_str; }

    static size_t msize()
    {
        size_t s = 0;
        for (auto it = s_interned.begin(); it != s_interned.end(); ++it)
            s += it->size();
        return s;
    }

private:
    static std::unordered_set <std::string> s_interned;
    static const char* make_intern(const std::string& value)
    {
        return s_interned.insert(value).first->c_str();
    }
    /*static const char* make_intern(const char * value)
    {
        return s_interned.insert(value).first->c_str();
    }*/

    std::string_view  m_str;
};


template <> struct std::hash<interned_string> 
{ 
    inline std::size_t operator() (const interned_string& s) const 
    { 
        return std::hash<const char*> {} (s.c_str());
    }
};


struct filestream
{
    static filestream* open(const char* path, const char* mode);
    static filestream* open_rb(const char* path);   // binary read only 
    static filestream* open_wb(const char* path);   // binary write

    ~filestream();

    uint32_t    read(uint32_t size, void* out_data);
    uint32_t    write(uint32_t size, const void* in_data);
    void        seek(uint32_t offset, int whence);
    void        flush();
   
    template<class T>
    inline T    read() { T res = {}; read(sizeof(T), &res); return res; }
 
    inline char* read_string(char* data) {
        data[0] = '\0';
        uint16_t len = read<uint16_t>();
        read(len, data);
        return data;
    }

    template<class T>
    inline void write(T v) { write(sizeof(T), &v); }

    inline void write_chunk_info(uint16_t type, uint16_t size) {
        write(type);
        write(size);
    }

    inline void write_chunk(uint16_t type, uint16_t size, const char* data) {
        write(type);
        write(size);
        write(size, data);
    }

    inline void write_string(uint16_t size, const char* data) {
        write(size);
        write(size, data);
    }/**/

private:
    filestream(struct stream_impl*);
    struct stream_impl* m_impl;
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
    inline bool             operator == (const Guid & other) const { return m_lo == other.m_lo && m_hi == other.m_hi; }
    inline bool             operator != (const Guid & other) const { return m_lo != other.m_lo && m_hi != other.m_hi; }
protected:
    union
    {
        struct { uint64_t m_lo, m_hi; };
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


#endif
