#ifndef __common_h__
#define __common_h__

#include <assert.h>
#include <string.h> // memcmp
#include <stdlib.h> // rand,clock
#include <stdarg.h> // va_arg
#include <stdint.h> // types
#include <time.h>   // time

#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <type_traits>
#include <functional>
#include <unordered_set>

#include "threads.h"

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


struct hasher
{
    static uint32_t     murmur32(const void* data, uint32_t size, uint32_t seed = 5381);
    static uint64_t     murmur64(const void *data, uint32_t size, uint32_t seed = 5381);
    static uint64_t     xxhash64(const void* input, size_t length, uint64_t seed);
    static size_t       bernstein_ci(const void* data, uint32_t size, uint32_t seed = 5381);
};

struct path
{
    static int copy_file(const char* src_path, const char* dest_path);
    static int ensure_directory_exists(char* canonical_dir_path);
    static uint32_t canonicalize_resource_path(const char* src_path, char* out_canonical, uint32_t max_size);
};

struct debug
{
    static void         log(const char* msg, ...);
    static void         log_error(const char* msg, ...);
    static void         log_warning(const char* msg, ...);

    static void         breakpoint();
    static int          callstack(uintptr_t * frames, uint32_t count);
    static void         callstack_names(uintptr_t * frames, uint32_t count, char** names = nullptr);
};


struct utf8
{
    static bool         is_ascii(const char* data, size_t size);
    static bool         is_ascii(const wchar_t* data, size_t size);

    static size_t       wchar_to_utf8(const wchar_t* data, size_t size, uint8_t* s);
    static size_t       utf8_to_wchar(const uint8_t* data, size_t size, wchar_t* w);

    static std::wstring from_utf8(const uint8_t* data, size_t size);
};


typedef struct guid_t {
    uint64_t    high;
    uint64_t    low;
} guid_t;


struct uuid
{
    static guid_t       generate_from_seed(size_t seed);
    static guid_t       generate_uuid_v4();

    static uint64_t     runtime_guid(guid_t _guid);

    static bool         is_guid_str(const char* buff);
    static guid_t       str_to_guid(const char * str);
    static char*        guid_to_str(guid_t g, char * buff);
};

inline bool operator==(const guid_t& a, std::nullptr_t) { return (a.high | a.low) == 0; }
inline bool operator!=(const guid_t& a, std::nullptr_t) { return (a.high | a.low) != 0; }
inline bool operator==(const guid_t& a, const guid_t& b) { return a.high == b.high && a.low == b.low; }
inline bool operator!=(const guid_t& a, const guid_t& b) { return !(a == b); }   // was '&&': guids differing in one half compared as equal

template<> struct std::hash<guid_t> {
    std::size_t operator()(const guid_t& g) const noexcept { 
        return std::hash<uint64_t>{}(g.high) ^ (std::hash<uint64_t>{}(g.low) << 1); 
    }
};

struct interned_string
{
public:
    interned_string() { clear(); }
    interned_string(const char* str) { m_ptr = make_intern(str); }
    interned_string(const std::string& str) { m_ptr = make_intern(str.data()); }
    interned_string(const std::string_view& str) { m_ptr = make_intern(std::string(str)); } // string_view is not NUL-terminated

    void                clear() { m_ptr = ""; }
    inline size_t       length() const { return strlen(m_ptr); }
    inline const char*  data()   const { return m_ptr; }
    inline const char*  c_str()  const { return m_ptr; }
    inline bool         empty()  const { return length() == 0; }

    inline friend bool operator == (const interned_string& b1, const interned_string& b2) { return b1.m_ptr == b2.m_ptr; }
    inline friend bool operator <  (const interned_string& b1, const interned_string& b2) { return b1.m_ptr < b2.m_ptr; }

    static size_t msize()
    {
        std::lock_guard<std::mutex> lock(pool_mutex());
        size_t s = 0;
        for (auto it = pool().begin(); it != pool().end(); ++it)
            s += it->size();
        return s;
    }

private:
    // function-local statics: no definition in .cpp (stb-style), no static init order issues
    static std::unordered_set<std::string>& pool()       { static std::unordered_set<std::string> s; return s; }
    static std::mutex&                      pool_mutex() { static std::mutex m; return m; }

    static const char* make_intern(const std::string& value)
    {
        std::lock_guard<std::mutex> lock(pool_mutex());
        return pool().insert(value).first->c_str();   // node-based set: c_str() pointers stay valid after rehash
    }
    const char * m_ptr = "";
    //std::string_view  m_str;
};


template <> struct std::hash<interned_string>
{
    inline std::size_t operator() (const interned_string& s) const
    {
        return std::hash<const char*> {} (s.c_str());
    }
};

struct binary_writer {
    uint32_t    size() const { return (uint32_t)buffer.size(); }
    const void* data() const { return buffer.data(); }
    void        clear()      { buffer.clear(); }

    void    write(uint32_t size, const void* data) {
        const char* p = (const char*)data;
        buffer.insert(buffer.end(), p, p + size);       // grows as needed (was fixed 512 bytes, silently dropped overflow)
    }

    template<class T> void write(const T& v) {
        static_assert(std::is_trivially_copyable_v<T>);
        write(sizeof(T), &v);
    }

private:
    std::vector<char> buffer;
};

struct filestream
{
    static filestream* open(const char* path, const char* mode);
    static filestream* open_rb(const char* path);   // binary read only 
    static filestream* open_wb(const char* path);   // binary write
    ~filestream();

    virtual uint32_t            read(uint32_t size, void* out_data);
    virtual uint32_t            write(uint32_t size, const void* in_data);
    virtual void                seek(uint32_t offset, int whence);
    virtual uint32_t            tell();
    virtual void                flush();

    template<class T>
    inline T            read() { T res = {}; read(sizeof(T), &res); return res; }

    template<class T>
    inline void         write(T v) { static_assert(std::is_trivially_copyable_v<T>); write(sizeof(T), &v); }

private:
    filestream(struct stream_impl*);
    struct stream_impl* m_impl;
};

template<>
inline void filestream::write<interned_string>(interned_string str) {
    write((uint16_t)str.length());
    write((uint32_t)str.length(), str.data());
}

template<>
inline interned_string filestream::read<interned_string>() {
    char buffer[2048] = "";
    uint16_t len = read<uint16_t>();
    assert(len < sizeof(buffer));
    read(len, buffer);
    return interned_string(buffer);
}

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
        double ms = std::chrono::duration<double, std::milli>(m_end - m_start).count();

        printf("%*s%s: %.3f ms\n", intend*4, "", m_msg, ms);
    }

private:
    const char * m_msg = "";
    static inline int intend = 0;
    std::chrono::high_resolution_clock::time_point m_start;
    std::chrono::high_resolution_clock::time_point m_end;
};


#endif
