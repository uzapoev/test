#ifndef __binary_serializer_h__
#define __binary_serializer_h__

// Binary twin of json_serializer.h: the same ReflectObject / ReflectObjectExternal descriptions, written positionally
// (no field names), so one struct description gives both the json reader and a compact binary format.
//
//   struct material_json { ...; ReflectObject2(material_json, shader, textures, vectors, scalars); };
//
//   binary_writer w;  binary::write_blob(w, 'MATB', 1, material);                  // compiler side
//   material_json m;  if (!binary::read_blob(data, size, 'MATB', 1, m)) ...        // runtime side, no allocations but the fields
//
// Supported field types: bool, enums, integers, floats (copied as is, little endian), std::string / interned_string,
// guid_t, std::vector<T> of any supported type, and nested reflected structs. Anything else fails to compile.
//
// Layout of a blob:  u32 magic | u16 version | u32 schema hash | payload
//   - the schema hash is built from field names and kinds of the struct (not from compiler type names, so it is the same for the
//     tool and the game on every platform). A blob compiled from an older struct layout is rejected instead of misread.
//   - strings and vectors are prefixed with a varint length, guid_t is 16 bytes.
// The reader never trusts the data: every read is bounds checked, a vector can not claim more elements than bytes left.

#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>
#include <type_traits>

#include "common.h"            // binary_writer, interned_string, guid_t
#include "json_serializer.h"   // ReflectObject*, reflection::

namespace binary
{
    template<class T> struct always_false : std::false_type {};

    template<class T> struct is_vector                      : std::false_type {};
    template<class T> struct is_vector<std::vector<T>>      : std::true_type  {};

    template<class T> constexpr bool is_reflected_v = !std::is_same_v<decltype(reflection::get_properties<T>()), void>;
    template<class T> constexpr bool is_string_v    = std::is_same_v<T, std::string> || std::is_same_v<T, interned_string>;

    // ------------------------------------------------------------------------------------------------------------------
    // writer
    // ------------------------------------------------------------------------------------------------------------------
    inline void write_varint(binary_writer& w, uint32_t v)
    {
        while (v >= 0x80) { w.write((uint8_t)(v | 0x80)); v >>= 7; }
        w.write((uint8_t)v);
    }

    template<class T> inline void write_value(binary_writer& w, const T& value)
    {
        if constexpr (std::is_same_v<T, bool>)              { w.write((uint8_t)(value ? 1 : 0)); }
        else if constexpr (std::is_enum_v<T>)               { w.write((std::underlying_type_t<T>)value); }
        else if constexpr (std::is_arithmetic_v<T>)         { w.write(value); }
        else if constexpr (is_string_v<T>)                  { write_varint(w, (uint32_t)value.length()); w.write((uint32_t)value.length(), value.data()); }
        else if constexpr (std::is_same_v<T, guid_t>)       { w.write(value.high); w.write(value.low); }
        else if constexpr (is_vector<T>::value)             { write_varint(w, (uint32_t)value.size()); for (const auto& e : value) write_value(w, e); }
        else if constexpr (std::is_array_v<T>)              { for (const auto& e : value) write_value(w, e); }       // all N elements
        else if constexpr (is_reflected_v<T>)
        {
            reflection::for_each(reflection::get_properties<T>(), [&](size_t, auto* field) { write_value(w, value.*field->member); });
        }
        else static_assert(always_false<T>::value, "binary serializer: unsupported field type");
    }

    // ------------------------------------------------------------------------------------------------------------------
    // reader
    // ------------------------------------------------------------------------------------------------------------------
    struct reader
    {
        const uint8_t*  p   = nullptr;
        const uint8_t*  end = nullptr;
        bool            ok  = true;

        reader() = default;
        reader(const void* data, size_t size) : p((const uint8_t*)data), end((const uint8_t*)data + size) {}

        size_t remaining() const { return (size_t)(end - p); }

        bool take(void* out, size_t n)
        {
            if (!ok || remaining() < n) { ok = false; return false; }
            memcpy(out, p, n);
            p += n;
            return true;
        }

        const uint8_t* take_span(size_t n)
        {
            if (!ok || remaining() < n) { ok = false; return nullptr; }
            const uint8_t* r = p;
            p += n;
            return r;
        }

        uint32_t varint()
        {
            uint32_t v = 0;
            for (int shift = 0; shift < 35; shift += 7) {
                uint8_t b = 0;
                if (!take(&b, 1)) return 0;
                v |= (uint32_t)(b & 0x7f) << shift;
                if (!(b & 0x80)) return v;
            }
            ok = false;     // more than 5 bytes
            return 0;
        }
    };

    template<class T> inline void read_value(reader& r, T& value)
    {
        if constexpr (std::is_same_v<T, bool>)              { uint8_t b = 0; r.take(&b, 1); value = b != 0; }
        else if constexpr (std::is_enum_v<T>)               { std::underlying_type_t<T> u = {}; r.take(&u, sizeof(u)); value = (T)u; }
        else if constexpr (std::is_arithmetic_v<T>)         { r.take(&value, sizeof(T)); }
        else if constexpr (is_string_v<T>)
        {
            const uint32_t len = r.varint();
            const uint8_t* s = r.take_span(len);
            if (s && len != 0) value = T(std::string((const char*)s, len));
            else               value = T();           // an empty interned_string is its default one (operator== compares pointers)
        }
        else if constexpr (std::is_same_v<T, guid_t>)       { r.take(&value.high, 8); r.take(&value.low, 8); }
        else if constexpr (is_vector<T>::value)
        {
            const uint32_t count = r.varint();
            value.clear();
            if (count > r.remaining()) { r.ok = false; return; }        // every element takes at least a byte
            value.resize(count);
            for (uint32_t i = 0; i < count && r.ok; ++i) read_value(r, value[i]);
        }
        else if constexpr (std::is_array_v<T>)
        {
            for (auto& e : value) { if (!r.ok) break; read_value(r, e); }
        }
        else if constexpr (is_reflected_v<T>)
        {
            reflection::for_each(reflection::get_properties<T>(), [&](size_t, auto* field) { read_value(r, value.*field->member); });
        }
        else static_assert(always_false<T>::value, "binary serializer: unsupported field type");
    }

    // ------------------------------------------------------------------------------------------------------------------
    // schema hash: names and kinds of the fields, recursively (FNV-1a)
    // ------------------------------------------------------------------------------------------------------------------
    inline uint32_t hash_bytes(uint32_t h, const void* data, size_t n)
    {
        const uint8_t* b = (const uint8_t*)data;
        for (size_t i = 0; i < n; ++i) { h ^= b[i]; h *= 16777619u; }
        return h;
    }

    template<class T> inline uint32_t hash_type(uint32_t h)
    {
        if constexpr (std::is_same_v<T, bool>)              { return hash_bytes(h, "b", 1); }
        else if constexpr (std::is_enum_v<T>)               { char k[2] = { 'e', (char)sizeof(T) }; return hash_bytes(h, k, 2); }
        else if constexpr (std::is_floating_point_v<T>)     { char k[2] = { 'f', (char)sizeof(T) }; return hash_bytes(h, k, 2); }
        else if constexpr (std::is_arithmetic_v<T>)         { char k[3] = { 'i', (char)sizeof(T), (char)std::is_signed_v<T> }; return hash_bytes(h, k, 3); }
        else if constexpr (is_string_v<T>)                  { return hash_bytes(h, "s", 1); }
        else if constexpr (std::is_same_v<T, guid_t>)       { return hash_bytes(h, "g", 1); }
        else if constexpr (is_vector<T>::value)             { return hash_type<typename T::value_type>(hash_bytes(h, "v", 1)); }
       // else if constexpr (is_small_array<T>::value)        { const uint32_t n = T::capacity(); h = hash_bytes(h, "a", 1); h = hash_bytes(h, &n, sizeof(n)); return hash_type<std::remove_cv_t<std::remove_reference_t<decltype(std::declval<T&>().data[0])>>>(h); }
        else if constexpr (std::is_array_v<T>)              { const uint32_t n = (uint32_t)std::extent_v<T>; h = hash_bytes(h, "A", 1); h = hash_bytes(h, &n, sizeof(n)); return hash_type<std::remove_extent_t<T>>(h); }
        else if constexpr (is_reflected_v<T>)
        {
            h = hash_bytes(h, "{", 1);
            reflection::for_each(reflection::get_properties<T>(), [&](size_t, auto* field) {
                using member_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<T&>().*field->member)>>;    // keeps T[N] an array
                h = hash_bytes(h, field->name, strlen(field->name) + 1);
                h = hash_type<member_type>(h);
            });
            return hash_bytes(h, "}", 1);
        }
        else { static_assert(always_false<T>::value, "binary serializer: unsupported field type"); return h; }
    }

    template<class T> inline uint32_t schema_hash() { return hash_type<T>(2166136261u); }

    // ------------------------------------------------------------------------------------------------------------------
    // blob = header + payload
    // ------------------------------------------------------------------------------------------------------------------
    template<class T> inline void write_blob(binary_writer& w, uint32_t magic, uint16_t version, const T& object)
    {
        w.write(magic);
        w.write(version);
        w.write(schema_hash<T>());
        write_value(w, object);
    }

    enum class blob_error { none, too_small, bad_magic, bad_version, bad_schema, bad_payload, trailing_bytes };

    // Returns blob_error::none on success. `object` is left in an unspecified (but valid) state on failure.
    template<class T> inline blob_error read_blob(const void* data, size_t size, uint32_t magic, uint16_t version, T& object)
    {
        reader r(data, size);
        uint32_t m = 0, h = 0;
        uint16_t v = 0;
        if (!r.take(&m, 4) || !r.take(&v, 2) || !r.take(&h, 4)) return blob_error::too_small;
        if (m != magic)             return blob_error::bad_magic;
        if (v != version)           return blob_error::bad_version;
        if (h != schema_hash<T>())  return blob_error::bad_schema;
        read_value(r, object);
        if (!r.ok)                  return blob_error::bad_payload;
        if (r.remaining() != 0)     return blob_error::trailing_bytes;
        return blob_error::none;
    }

    inline const char* blob_error_string(blob_error e)
    {
        switch (e) {
            case blob_error::none:           return "ok";
            case blob_error::too_small:      return "file is too small";
            case blob_error::bad_magic:      return "not a blob of this type";
            case blob_error::bad_version:    return "format version differs, recompile the asset";
            case blob_error::bad_schema:     return "struct layout changed, recompile the asset";
            case blob_error::bad_payload:    return "data is truncated or corrupt";
            case blob_error::trailing_bytes: return "unexpected bytes after the data";
        }
        return "?";
    }
}

#endif
