#ifndef __json_serializer_h__
#define __json_serializer_h__

#include <string>
#include <vector>
#include <set>
#include <charconv>

#include <json/sajson.h>
#include "common.h"

/*  struct UserInfo
    {
        std::string  pid;
        std::string  nick;
        std::string  image;
        std::string  url;

        ReflectObject(UserInfo,
            ReflectObjectFieldWithKey("id",  pid  ),
            ReflectObjectFieldWithKey("n",   nick ),
            ReflectObjectFieldWithKey("url", url  )
        );
    };
    auto user_info = json::from_json_string<UserInfo>(data.size(), data.data());
    auto str_data = json::to_json_string(user_info);

    struct vec3
    {
        float x,y,z;
    }
    ReflectObjectExternal(vec3, x, y, z );
*/

typedef enum field_options {
    field_options_none,
    field_options_asset_ref,
    field_options_read_only,
    field_options_hide_in_inspector,
} field_options;


#define _REFLECT_EXPAND_ARGS(x) x

#define _REFLECT_EXPAND_1(x)          ReflectObjectField(x)
#define _REFLECT_EXPAND_2(x, ...)     ReflectObjectField(x), _REFLECT_EXPAND_ARGS(_REFLECT_EXPAND_1(__VA_ARGS__))
#define _REFLECT_EXPAND_3(x, ...)     ReflectObjectField(x), _REFLECT_EXPAND_ARGS(_REFLECT_EXPAND_2(__VA_ARGS__))
#define _REFLECT_EXPAND_4(x, ...)     ReflectObjectField(x), _REFLECT_EXPAND_ARGS(_REFLECT_EXPAND_3(__VA_ARGS__))
#define _REFLECT_EXPAND_5(x, ...)     ReflectObjectField(x), _REFLECT_EXPAND_ARGS(_REFLECT_EXPAND_4(__VA_ARGS__))
#define _REFLECT_EXPAND_6(x, ...)     ReflectObjectField(x), _REFLECT_EXPAND_ARGS(_REFLECT_EXPAND_5(__VA_ARGS__))
#define _REFLECT_EXPAND_7(x, ...)     ReflectObjectField(x), _REFLECT_EXPAND_ARGS(_REFLECT_EXPAND_6(__VA_ARGS__))
#define _REFLECT_EXPAND_8(x, ...)     ReflectObjectField(x), _REFLECT_EXPAND_ARGS(_REFLECT_EXPAND_7(__VA_ARGS__))

#define _REFLECT_GET_MACRO(_1,_2,_3,_4,_5,_6,_7,_8, NAME, ...) NAME

#define _REFLECT_EXPAND_FIELDS(...) \
    _REFLECT_EXPAND_ARGS(_REFLECT_GET_MACRO(__VA_ARGS__, \
        _REFLECT_EXPAND_8, _REFLECT_EXPAND_7, _REFLECT_EXPAND_6, _REFLECT_EXPAND_5, \
        _REFLECT_EXPAND_4, _REFLECT_EXPAND_3, _REFLECT_EXPAND_2, _REFLECT_EXPAND_1)(__VA_ARGS__))

#define ReflectObject( CLASS, ... )                 public: static auto reflection_properties() { using Type = CLASS; return std::make_tuple(__VA_ARGS__); }   
#define ReflectObjectInherited( CLASS, BASE, ... )  public: static auto reflection_properties() { using Type = CLASS; return std::tuple_cat(std::make_tuple(__VA_ARGS__), reflection::get_properties<BASE>()); }
#define ReflectObjectExternal( CLASS, ...)          template <> inline auto reflection_properties<CLASS>() { using Type = CLASS; return std::make_tuple(__VA_ARGS__); }

#define ReflectObject2( CLASS, ... )                public: static auto reflection_properties() { using Type = CLASS; return std::make_tuple(_REFLECT_EXPAND_FIELDS(__VA_ARGS__)); }   
#define ReflectObjectExternal2( CLASS, ... )        template <> inline auto reflection_properties<CLASS>() { using Type = CLASS; return std::make_tuple(_REFLECT_EXPAND_FIELDS(__VA_ARGS__)); }


#define ReflectObjectField(FIELD)                   reflection::make_property(&Type::FIELD, #FIELD)
#define ReflectObjectFieldWithKey(KEY, FIELD)       reflection::make_property(&Type::FIELD, KEY)

template<class T> inline auto                       reflection_properties() {  } // should be empty, if object doesn't have reflection data it will return "void"


namespace reflection
{
    struct base_field_info_t {
        const char* name = "";
        const char* type_name = "";
        const int   type_index = -1;
    };

    template<typename Class, typename T>
    struct field_info_t : base_field_info_t
    {
        T Class::* member = nullptr;
    };

    template<typename Class, typename T>
    struct property
    {
        Class* object = nullptr;
        field_info_t<Class, T>* field = nullptr;

        constexpr bool is_valid() const noexcept { return object && field; }

        void set(const T& value)
        {
            if (is_valid())
                object->*field->member = value;
        }

        T get() const
        {
            if (is_valid())
                return object->*field->member;
            return T{};
        }
    };

    //tuple_for_each(T::properties(), [&](size_t idx, auto * arg){});
    template<size_t I = 0, typename Tuple, typename Func>
    inline void for_each(const Tuple& tuple, Func&& func) {
        if constexpr (I < std::tuple_size_v<Tuple>) {
            func(I, &std::get<I>(tuple));
            for_each<I + 1>(tuple, std::forward<Func>(func));
        }
    }

    struct type_registry {
        static inline std::atomic<int> g_type_counter { 0 };
        template<class T> static inline int get_type_index() noexcept {
            static int index = g_type_counter.fetch_add(1, std::memory_order_acq_rel);
            return index;
        }
     };

    template <typename T>
    class has_properties
    {
    private:
        template <typename C> static std::true_type  test(decltype(&C::reflection_properties)*);
        template <typename C> static std::false_type test(...);

    public:
        static constexpr bool value = decltype(test<T>(nullptr))::value;
    };

    template <typename T, typename std::enable_if_t<has_properties<T>::value>* = nullptr>
    auto get_properties() { return T::reflection_properties(); }

    template <typename T, typename std::enable_if_t<!has_properties<T>::value>* = nullptr>
    auto get_properties() { return reflection_properties<T>(); }


    template<typename Class, typename T>
    auto make_property(T Class::* member, const char* name)
    {
        auto type_name = typeid(T).name();
        const auto type_idx = reflection::type_registry::get_type_index<T>();
        return field_info_t<Class, T> { name, type_name, type_idx, member };
    }

    template<class T, class U>
    auto find_property(T* obj, const char* name)
    {
        auto properties = get_properties<T>();// T::properties();

        const base_field_info_t* base_field = nullptr;
        for_each(properties, [&](size_t idx, auto* arg) {
            if (!strcmp(name, arg->name))
                base_field = arg;
         });

        auto field_type_index = type_registry::get_type_index<U>();

        if (base_field != nullptr && field_type_index == base_field->type_index)
        {
            auto field = static_cast<field_info_t<T, U>*>(const_cast<base_field_info_t*>(base_field));
            return property<T, U> { obj, field };
        }

        return property<T, U> { nullptr, nullptr };
    }

    template<class CLASS, class TYPE>
    void set_property_value(CLASS* obj, const char* name, const TYPE& value)
    {
        auto prop = reflection::find_property<CLASS, TYPE>(obj, name);
        prop.set(value);
    }
}


namespace json
{
    template<class T> inline T              from_json_string(const std::string& jstr);
    template<class T> inline T              from_json_string(size_t size, char * data);
    template<class T> inline std::string    to_json_string(T& oject);


    class json_writer {
    public:
        std::vector<char> buffer;
        bool first_element = true;

        json_writer(size_t reserve_size = 1024 * 1024 * 1) {
            buffer.reserve(reserve_size);
        }

        void start_object() { write_char('{'); first_element = true; }
        void end_object()   { write_char('}'); first_element = false; }
        void start_array()  { write_char('['); first_element = true; }
        void end_array()    { write_char(']'); first_element = false; }

        void write_char(char c) {
            buffer.push_back(c);
        }

        void write_str_raw(const char* str, size_t len) {
            buffer.insert(buffer.end(), str, str + len);
        }

        void add_key(const char* key) {
            if (!first_element) write_char(',');
            write_char('"');
            write_str_raw(key, strlen(key));
            write_char('"');
            write_char(':');
            first_element = false;
        }

        // json string: quote, backslash and control characters (a raw newline is not legal json) are escaped
        void write_string_value(const char* value) {
            write_char('"');
            while (*value) {
                const unsigned char c = (unsigned char)*value;
                switch (c) {
                    case '"':   write_str_raw("\\\"", 2); break;
                    case '\\':  write_str_raw("\\\\", 2); break;
                    case '\n':  write_str_raw("\\n", 2);  break;
                    case '\r':  write_str_raw("\\r", 2);  break;
                    case '\t':  write_str_raw("\\t", 2);  break;
                    case '\b':  write_str_raw("\\b", 2);  break;
                    case '\f':  write_str_raw("\\f", 2);  break;
                    default:
                        if (c < 0x20) {
                            char esc[8];
                            int n = snprintf(esc, sizeof(esc), "\\u%04x", (unsigned)c);
                            write_str_raw(esc, (size_t)n);
                        } else {
                            write_char((char)c);
                        }
                }
                value++;
            }
            write_char('"');
        }

        void write_float_value(float value) {
            char local_buf[64];
            auto [ptr, ec] = std::to_chars(local_buf, local_buf + sizeof(local_buf), value);
            if (ec == std::errc()) {
                write_str_raw(local_buf, ptr - local_buf);
            }
            else {
                int len = snprintf(local_buf, sizeof(local_buf), "%.6f", value);
                write_str_raw(local_buf, len);
            }
        }

        void write_int_value(int32_t value) {
            char local_buf[32];
            auto [ptr, ec] = std::to_chars(local_buf, local_buf + sizeof(local_buf), value);
            write_str_raw(local_buf, ptr - local_buf);
        }

        // array / list element separator: call before every element written without add_key()
        void next_element() {
            if (!first_element) write_char(',');
            first_element = false;
        }
    };

    template<class T> inline void serialize_json(T& object, json_writer& writer);

    // Context of a read: whatever the caller wants to be handed down to every object being read (world, scene, ...).
    // `no_ctx` for a caller that has none: deserialize_sajson(object, value) is deserialize_sajson(object, value, no_ctx{}).
    struct no_ctx {
        template<class T, class Ctx> 
        void unused(T& t, const sajson::string& k, const sajson::value& v, Ctx&) {};

        template<class T>
        T * allocate() { return new T(); }
    };

    template<class T, class Ctx>
    inline void deserialize_sajson(T& object, const sajson::value& jvalue, Ctx& ctx);

    namespace detail
    {
        template<class T>
        struct is_vector                    { static bool const value = false;        };

        template<class T> 
        struct is_vector<std::vector<T> >   { static bool const value = true;        };


        inline bool key_equals(const sajson::string& key, const char* name)
        {
            const size_t length = strlen(name);
            return key.length() == length && memcmp(key.data(), name, length) == 0;
        }

        template <class T, class Ctx> inline void read_vector(const sajson::value & obj, std::vector<T>& value, Ctx& ctx)
        {
            value.resize(obj.get_length());
            for(size_t i = 0; i < obj.get_length(); ++i)
            {
                deserialize_sajson(value[i], obj.get_array_element(i), ctx);
            }
        }
    }

 
    template<class T, class Ctx>
    inline void deserialize_sajson(T& object, const sajson::value& jvalue, Ctx& ctx)
    {
        if constexpr (!std::is_same_v<decltype(reflection::get_properties<T>()), void>)
        {
            size_t found = 0;                         // keys of the json that are fields of T
            auto properties = reflection::get_properties<T>();

            reflection::for_each(properties, [&](size_t idx, auto* arg) {

                sajson::string key(arg->name, strlen(arg->name));

                const size_t at = jvalue.find_object_key(key);
                if (at >= jvalue.get_length())
                    return;                                 // no such key
                ++found;                                    // the key is in the json (a json null is a key too)

                auto value = jvalue.get_object_value(at);
                if (value.get_type() == sajson::TYPE_NULL)
                    return;

                using member_type = std::decay_t<decltype(object.*arg->member)>;

                if constexpr (std::is_assignable_v<member_type, std::string>)   // string, string_view
                {
                    object.*arg->member = value.as_cstring();
                }
                else if constexpr (std::is_floating_point_v<member_type>)        // float, double
                {
                    object.*arg->member = (member_type)value.get_number_value();
                }
                else if constexpr (std::is_same_v<member_type, bool>)           // json true/false (a number is accepted too)
                {
                    object.*arg->member = value.get_type() == sajson::TYPE_TRUE ||
                                          (value.get_type() == sajson::TYPE_INTEGER && value.get_integer_value() != 0);
                }
                else if constexpr (std::is_integral_v<member_type>)             // int, int16m int32...
                {
                    object.*arg->member = (member_type)value.get_integer_value();
                }
                else if constexpr (detail::is_vector<member_type>::value)       // vector
                {
                    detail::read_vector(value, object.*arg->member, ctx);
                }
                else if constexpr (std::is_same_v<member_type, guid_t>)
                {
                    const char* str = value.as_cstring();
                    object.*arg->member = uuid::str_to_guid(str);
                } 
                else if constexpr (!std::is_same_v<decltype(reflection::get_properties<member_type>()), void>)
                {
                    deserialize_sajson(object.*arg->member, value, ctx);
                }
                else
                {
                    debug::log_error("[json_serializer]:unknmown type \'%s\'", typeid(member_type).name());
                }
            }); // for_each
  

            // keys without a field: the context takes them (ctx.unused(object, key, value)), else it is reported
            // (all the keys are fields in most objects: nothing to look for then)
            const size_t count = jvalue.get_length();
            if (found < count)
            {
                for (size_t i = 0; i < count; ++i)
                {
                    sajson::string key = jvalue.get_object_key(i);
                    sajson::value value = jvalue.get_object_value(i);

                    bool is_field = false;
                    reflection::for_each(properties, [&](size_t, auto* arg) {
                        if (detail::key_equals(key, arg->name)) is_field = true;
                    });
                    if (is_field)
                        continue;

                    ctx.unused(object, key, value, ctx);
                }
            }
        }
    }

    // without a context: the callers that don't need one
    template<class T>
    inline void deserialize_sajson(T& object, const sajson::value& jvalue)
    {
        no_ctx ctx;
        deserialize_sajson(object, jvalue, ctx);
    }

    namespace detail
    {
        // one value of a json array (vector element): string, guid (as text), number, bool or a reflected struct
        template<class T> inline void write_element(const T& value, json_writer& writer)
        {
            writer.next_element();
            if constexpr (std::is_same_v<T, bool>) {
                if (value) writer.write_str_raw("true", 4); else writer.write_str_raw("false", 5);
            } else if constexpr (std::is_same_v<T, guid_t>) {
                char buf[64] = {};
                uuid::guid_to_str(value, buf);
                writer.write_string_value(buf);
            } else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, interned_string>) {
                writer.write_string_value(value.c_str());
            } else if constexpr (std::is_floating_point_v<T>) {
                writer.write_float_value((float)value);
            } else if constexpr (std::is_integral_v<T>) {
                writer.write_int_value((int32_t)value);
            } else {
                T copy = value;
                serialize_json(copy, writer);   // writer state is restored by start_object/end_object
            }
        }
    }

    // Writes the reflected fields of `object` as a json object: bool, integers, floats, strings, guids (as text), vectors
    // and nested reflected structs. (Vectors are written as json arrays, what deserialize_sajson reads back for reflected elements.)
    template<class T>
    inline void serialize_json(T& object,  json_writer & writer)
    {
        if constexpr (!std::is_same_v<decltype(reflection::get_properties<T>()), void>)
        {
            writer.start_object();
            reflection::for_each(reflection::get_properties<T>(), [&](size_t idx, auto* arg) {

                using member_type = std::decay_t<decltype(object.*arg->member)>;

                if constexpr (std::is_same_v<member_type, bool>) {
                    writer.add_key(arg->name);
                    if (object.*arg->member) writer.write_str_raw("true", 4); else writer.write_str_raw("false", 5);
                } else if constexpr (std::is_assignable_v<member_type&, std::string>) {
                    writer.add_key(arg->name);
                    writer.write_string_value((object.*arg->member).c_str());
                } else if constexpr (std::is_floating_point_v<member_type>) {
                    writer.add_key(arg->name);
                    writer.write_float_value((float)(object.*arg->member));
                } else if constexpr (std::is_integral_v<member_type>) {
                    writer.add_key(arg->name);
                    writer.write_int_value((int32_t)(object.*arg->member));
                } else if constexpr (std::is_same_v<member_type, guid_t>) {
                    char buf[64] = {};
                    uuid::guid_to_str(object.*arg->member, buf);
                    writer.add_key(arg->name);
                    writer.write_string_value(buf);
                } else if constexpr (detail::is_vector<member_type>::value) {
                    writer.add_key(arg->name);
                    writer.start_array();
                    for (const auto& e : object.*arg->member) detail::write_element(e, writer);
                    writer.end_array();
                } else if constexpr (!std::is_same_v<decltype(reflection::get_properties<member_type>()), void>) {
                    writer.add_key(arg->name);
                    serialize_json(object.*arg->member, writer);
                }
            });
            writer.end_object();
        }
    }


    template<class T>
    inline T from_json_string(const std::string& jstr)
    {
        T object = {};

        sajson::document doc = sajson::parse(sajson::dynamic_allocation(),
            sajson::mutable_string_view(jstr.size(), (char*)jstr.data()));

        deserialize_sajson(object, doc.get_root());
        return std::move(object);
    }

    template<class T>
    inline T from_json_string(size_t size, char * data)
    {
        T object = {};

        sajson::document doc = sajson::parse(   sajson::dynamic_allocation(),
                                                sajson::mutable_string_view(size, data));
        deserialize_sajson(object, doc.get_root());
        return std::move(object);
    }

    template<class T, class Ctx>
    inline T from_json_string_ctx(size_t size, char* data, Ctx & ctx)
    {
        T object = {};

        sajson::document doc = sajson::parse(sajson::dynamic_allocation(),
            sajson::mutable_string_view(size, data));
        deserialize_sajson(object, doc.get_root(), ctx);
        return std::move(object);
    }

    template<class T>
    inline std::string to_json_string(T& object)
    {
        json_writer writer(1024);
        serialize_json(object, writer);
        return std::string(writer.buffer.begin(), writer.buffer.end());
    }
}


#endif

