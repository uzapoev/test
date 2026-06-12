#ifndef __json_serializer_h__
#define __json_serializer_h__

#include <cstdint>
#include <cmath>
#include <cctype>
#include <string>
#include <vector>

#include <string>
#include <charconv>

#include <json/sajson.h>
#include "common.h"


/*  struct UserInfo
    {
        std::string  pid;
        std::string  nick;
        std::string  image;
        std::string  url;

        JsonSerialize(UserInfo,
            SerializeFieldWithKey("id",  pid  ),
            SerializeFieldWithKey("n",   nick ),
            SerializeFieldWithKey("url", url  )
        );
    };
    auto userinfo = json::from_json<UserInfo>(data);

    struct RenderMeshComponent
    {
        guid    mesh_guid;
        guid*  material_guids;

        ReflectObject(RenderMeshComponent,
            ReflectField("mesh",        mesh_guid ),
            ReflectField("materials",   material_guids )
        );
    };

    struct vec3
    {
        float x,y,z;
    }

    JsonSerializeExternal(vec3, SerializeField(x), 
                                SerializeField(y), 
                                SerializeField(z) );
*/

typedef enum field_options {
    field_options_none,
    field_options_read_only,
    field_options_hide_in_inspector,
} field_options;

#define ReflectObject( CLASS, ... )                 public: static auto reflection_properties() { using Type = CLASS; return std::make_tuple(__VA_ARGS__); }   
#define ReflectObjectInherited( CLASS, BASE, ... )  public: static auto reflection_properties() { using Type = CLASS; return std::tuple_cat(std::make_tuple(__VA_ARGS__), BASE::reflection_properties()); }
#define ReflectObjectExternal( CLASS, ...)          template <> inline auto reflection_properties<CLASS>() { using Type = CLASS;  return std::make_tuple(__VA_ARGS__); }

#define ReflectField(FIELD)                       reflection::make_property(&Type::FIELD, #FIELD)
#define ReflectFieldWithKey(KEY, FIELD)           reflection::make_property(&Type::FIELD, KEY)

template<class T> inline auto                     reflection_properties() { /*return std::make_tuple();*/ }


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

        auto field_type_index =  type_registry::get_type_index<U>();

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
        void end_object() { write_char('}'); first_element = false; }
        void start_array() { write_char('['); first_element = true; }
        void end_array() { write_char(']'); first_element = false; }

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

        void write_string_value(const char* value) {
            write_char('"');
            while (*value) {
                char c = *value;
                if (c == '"' || c == '\\') { write_char('\\'); }
                write_char(c);
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
    };

    namespace detail
    {


        template<class T, typename Enable = void>
        struct is_vector {
            static bool const value = false;
        };

        template<class T>
        struct is_vector<std::vector<T> > {
            static bool const value = true;
        };

        template <class T> inline void read_vector(const sajson::value & obj, std::vector<T>& value)
        {
            for(size_t i = 0; i < obj.get_length(); ++i)
            {
                T tmp_value = {};
                deserialize_sajson(tmp_value, obj.get_array_element(i));
                value.push_back(std::move(tmp_value));
            }
        }
    }

 
    template<class T>
    inline void deserialize_sajson(T& object, const sajson::value& jvalue)
    {
        if constexpr (!std::is_same_v<decltype(reflection::get_properties<T>()), void>)
        {
            auto properties = reflection::get_properties<T>();
            reflection::for_each(properties, [&](size_t idx, auto* arg) {

                sajson::string key(arg->name, strlen(arg->name));

                auto value = jvalue.get_value_of_key(key);
                if (value.get_type() == sajson::TYPE_NULL)
                    return;

                using member_type = std::decay_t<decltype(object.*arg->member)>;

               if constexpr (std::is_assignable_v<member_type, std::string>)   // string, string_view
                {
                    const char* str = value.as_cstring();
                    object.*arg->member = str;
                }
                if constexpr (std::is_floating_point_v<member_type>)        // float, double
                {
                    object.*arg->member = (member_type)value.get_number_value();
                }
                if constexpr (std::is_integral_v<member_type>)             // bool, int, int16m int32...
                {
                    object.*arg->member = (member_type)value.get_integer_value();
                }
                if constexpr (detail::is_vector<member_type>::value)       // vector
                {
                    detail::read_vector(value, object.*arg->member);
                }
                if constexpr (!std::is_same_v<decltype(reflection::get_properties<member_type>()), void>)
                {
                    deserialize_sajson(object.*arg->member, value);
                }
            }); // for_each
        }
    }

    template<class T>
    inline void serialize_json(T& object,  json_writer & writer)
    { 
        if constexpr (!std::is_same_v<decltype(detail::get_properties<T>()), void>)
        {
            detail::tuple_for_each(detail::get_properties<T>(), [&](size_t idx, auto* arg) {

               using member_type = std::decay_t<decltype(object.*arg->member)>;
                if constexpr (std::is_assignable_v<member_type, std::string>)   // string, string_view
                {
                    writer.add_key(arg->name);
                    writer.write_string_value(object.*arg->member.c_str());
                }
                else
                {
                    debug::breakpoint();
                }
            });
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

    template<class T>
    inline std::string to_json_string(T& object)
    {
        json_writer writer;
        std::string jdata;
        tuple_for_each(T::properties(), [&](size_t idx, auto* arg) { 
            json::detail::jsonwrite(jdata, arg->name, object.*(arg->member)); 
        });
        return std::move(jdata);
    }
}


#endif

