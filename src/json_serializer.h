#ifndef __json_serializer_h__
#define __json_serializer_h__

//#include "json.h"

#include <cstdint>
#include <cmath>
#include <cctype>
#include <string>
#include <deque>
#include <map>
#include <type_traits>
#include <initializer_list>
#include <ostream>
#include <iostream>
#include <sstream>
#include <vector>

#include "common.h"
#include "gason.h"

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


    struct vec3
    {
        float x,y,z;
    }

    JsonSerializeExternal(vec3, SerializeField(x), 
                                SerializeField(y), 
                                SerializeField(z) );
*/

#define JsonSerialize( CLASS, ... )                 public: static auto jproperties() { using Type = CLASS; return std::make_tuple(__VA_ARGS__); }   
#define JsonSerializeInherited( CLASS, BASE, ... )  public: static auto jproperties() { using Type = CLASS; return std::tuple_cat(std::make_tuple(__VA_ARGS__), BASE::properties()); }
#define JsonSerializeExternal( CLASS, ...)          template <> inline auto jproperties<CLASS>() { using Type = CLASS;  return std::make_tuple(__VA_ARGS__); }

#define SerializeField(FIELD)                       json::make_property(&Type::FIELD, #FIELD)
#define SerializeFieldWithKey(KEY, FIELD)           json::make_property(&Type::FIELD, KEY)

template<class T> inline auto                       jproperties() { }

namespace json
{
    template<class T> inline T              from_json_string(const std::string& jstr);
  //  template<class T> inline json::JSON     to_json_string(T& oject);
   // template<class T> inline json::JSON     to_json(T& oject);

    namespace detail
    {
        template <typename T>
        class has_properties
        {
        private:
            typedef char yes[1];
            typedef char no[2];
            
            template <typename C> static yes& test(decltype(&C::jproperties)){};
            template <typename C> static no&  test(...){};

        public:
            enum { value = sizeof(test<T>(0)) == sizeof(yes) };
        };


        static int _type_index_counter = 0;
    
        template<class T> inline int jtype_index() noexcept
        {
            static int index = ++_type_index_counter;
            return index;
        }
   

        //tuple_for_each(T::properties(), [&](size_t idx, auto * arg){});
        template<std::size_t I = 0, typename Tuple, typename Func>
        typename std::enable_if< I != std::tuple_size<Tuple>::value, void >::type
            inline tuple_for_each(const Tuple& tuple, Func&& func)
        {
            func(I, &std::get<I>(tuple));
            tuple_for_each<I + 1>(tuple, func);
        }

        template<std::size_t I = 0, typename Tuple, typename Func>
        typename std::enable_if< I == std::tuple_size<Tuple>::value, void >::type
            inline tuple_for_each(const Tuple& tuple, Func&& func) {}


      //  #define jprop   typename std::enable_if_t<has_properties<T>::value>
      //  #define jnoprop typename std::enable_if_t<!has_properties<T>::value>

        typedef struct base_field_info_t {
            const char* name        = "";
            const char* type_name   = "";
            const int   type_index  = -1;
        } base_field_info_t;

        template<typename Class, typename T>
        struct field_info_t : base_field_info_t
        {
            T Class::*member        = nullptr;
        };


        template<class T, typename Enable = void>
        struct is_vector {
            static bool const value = false;
        };

        template<class T>
        struct is_vector<std::vector<T> > {
            static bool const value = true;
        };
        /*

        template <class T> inline void jsonread_vec(const json::JSON& j, std::vector<T>& value)
        {
            for (int i = 0; i < j.length(); ++i)
            {
                T tmpvalue = {};
                deserialize(tmpvalue, j.at(i));
                value.push_back(std::move(tmpvalue));
            }
        }
        */

        template <class T> inline void jsonread_vec2(const JsonValue& jvalue, std::vector<T>& value)
        {
            auto tag = jvalue.getTag();
            auto n = jvalue.toNode();

            for (auto i : jvalue)
            {
                T tmpvalue = {};
                deserialize2(tmpvalue, i->value);
                value.push_back(std::move(tmpvalue));
            }
          /*  while(n->next != nullptr)
            {
                n = n->next;
            }
            for (int i = 0; i < j.length(); ++i)
            {
                T tmpvalue = {};
                //deserialize2(tmpvalue, j.at(i));
                value.push_back(std::move(tmpvalue));
            }*/
        }

     /*   template <class T> void jsonwrite(json::JSON& j, const std::string& key, T& value)            { j[key] = json::to_json(value);   }
        template <> inline void jsonwrite(json::JSON& j, const std::string& key, std::string& value)  { j[key] = value; }
        template <> inline void jsonwrite(json::JSON& j, const std::string& key, float& value)        { j[key] = value; }
        template <> inline void jsonwrite(json::JSON& j, const std::string& key, double& value)       { j[key] = value; }
        template <> inline void jsonwrite(json::JSON& j, const std::string& key, int& value)          { j[key] = value; }
        template <> inline void jsonwrite(json::JSON& j, const std::string& key, long& value)         { j[key] = value; }

        template <class T> typename std::enable_if_t<has_properties<T>::value>
        inline jsonwrite(json::JSON& j, const std::string& k, std::vector<T>& v)
        {
            auto arr = json::Array();
            for (size_t i = 0; i < v.size(); ++i) 
            {
                auto newjdata = to_json(v[i]);
                arr.append(newjdata);
            }
            j[k] = arr;
        }

        template <class T> inline jnoprop jsonwrite(json::JSON& j, const std::string& k, std::vector<T>& v) 
        {
            auto arr = json::Array();
            for (size_t i = 0; i < v.size(); ++i) 
            {
                arr.append(v[i]);
            }
            j[k] = arr;
        }*/


        template <typename T, typename std::enable_if_t<has_properties<T>::value>* = nullptr>
        auto get_properties() { return T::jproperties(); }

        template <typename T, typename std::enable_if_t<!has_properties<T>::value>* = nullptr>
        auto get_properties() { return jproperties<T>(); }
    }

    template<typename Class, typename T>
    auto make_property(T Class::*member, const char* name) 
    {
        auto type_name = typeid(T).name();
        auto type_idx = detail::jtype_index<T>();
        return detail::field_info_t<Class, T> {name, type_name, type_idx, member};
    }

    
    template<class T>
    inline void deserialize2(T& object, const JsonValue& jvalue)
    {
        if constexpr (!std::is_same_v<decltype(detail::get_properties<T>()), void>)
        {
            detail::tuple_for_each(detail::get_properties<T>(), [&](size_t idx, auto* arg)
                {
                    if (!jvalue.hasKey(arg->name))
                        return;

                    auto node = jvalue.at(arg->name);
                    using member_type = std::decay_t<decltype(object.*arg->member)>;

                    if constexpr (std::is_assignable_v<member_type, std::string>)   // string, string_view
                    {
                        const char * str= node->value.toString();
                        object.*arg->member = node->value.toString();
                    }
                    else if constexpr (std::is_floating_point_v<member_type>)        // float, double
                    {
                        object.*arg->member = (member_type)node->value.toNumber();
                    }
                   /* else if constexpr (std::is_integral_v<member_type>)             // bool, int, int16m int32...
                    {
                        object.*arg->member = (member_type)value.ToInt();
                    } */
                    else if constexpr (detail::is_vector<member_type>::value)       // vector
                    {
                        detail::jsonread_vec2(node->value, object.*arg->member);
                    }
                    else if constexpr (!std::is_same_v<decltype(detail::get_properties<member_type>()), void>)
                    {
                        deserialize2(object.*arg->member, node->value);
                    }
                });
        }
    }

    /*
    template<class T>
    inline void deserialize(T& object, const json::JSON& jdata)
    {
        if constexpr(!std::is_same_v<decltype(detail::get_properties<T>()), void>)
        {
            detail::tuple_for_each(detail::get_properties<T>(), [&](size_t idx, auto* arg)
            {
                if(!jdata.hasKey(arg->name) )
                    return;
               
                auto & jobject = jdata.at(arg->name);
                using member_type = std::decay_t<decltype(object.*arg->member)>;
             
                if constexpr (std::is_assignable_v<member_type, std::string>)   // string, string_view
                {
                    object.*arg->member = jobject.ToString();
                }
                else if constexpr(std::is_floating_point_v<member_type>)        // float, double
                {
                    object.*arg->member = (member_type)jobject.ToFloat();
                }
                else if constexpr (std::is_integral_v<member_type>)             // bool, int, int16 int32...
                {
                    object.*arg->member = (member_type)jobject.ToInt();
                }
                else if constexpr (detail::is_vector<member_type>::value)       // vector
                {
                    detail::jsonread_vec(jobject, object.*arg->member);
                }
                else if constexpr(!std::is_same_v<decltype(detail::get_properties<member_type>()), void>)
                {
                    deserialize(object.*arg->member, jobject);
                }
            });
        }
    }*/

    template<class T>
    inline T from_json_string(char* ptr, size_t size)
    {
        T object;

        JsonAllocator allocator;
        JsonValue value;
        char* endptr = nullptr;
        auto r = jsonParse(ptr, &endptr, &value, allocator);
        deserialize2(object, value);
        return std::move(object);
    }

    template<class T>
    inline T from_json_string(const std::string& jstr)
    {
        T object;

        JsonAllocator allocator;
        JsonValue value;

        char* ptr = (char*)jstr.data();
        char* endptr = nullptr;
        auto r = jsonParse(ptr, &endptr, &value, allocator);
        deserialize2(object, value);
        return std::move(object);
      /*  auto t = value.getTag();

        value.hasKey("childs");
        auto n = value.toNode();
        */

    //    auto jdata = json::JSON::Load(jstr);

     //   deserialize(object, jdata);

     //   return std::move(object);
    }
/*
    template<class T>
    inline json::JSON to_json_string(T& object)
    {
        json::JSON jdata;
        tuple_for_each(T::properties(), [&](size_t idx, auto* arg) { 
            json::detail::jsonwrite(jdata, arg->name, object.*(arg->member)); 
        });
        return std::move(jdata);
    }

    template<class T> inline json::JSON to_json(T& oject)
    {
        return {};
    }*/
}



namespace reflection
{
    template<typename Class, typename T>
    struct property
    {
        Class*                                  object = nullptr;
        json::detail::field_info_t<Class, T>*   field  = nullptr;

        void set(const T& value)
        {
            if (object && field)
                object->*field->member = value;
        }
    };

    template<class T, class U>
    auto find_property(T* obj, const char* name)
    {
        auto properties = json::detail::get_properties<T>();// T::properties();

        const json::detail::base_field_info_t* base_field = nullptr;
        tuple_for_each(properties, [&](size_t idx, auto* arg) {
            if (!strcmp(name, arg->name))
                base_field = arg;
            });

        auto field_type = typeid(U).name();

        if (base_field != nullptr && !strcmp(field_type, base_field->type_name))
        {
            auto field = (json::detail::field_info_t<T, U>*)base_field;
            return property<T, U>{ obj, field };
        }

        return property<T, U>{ nullptr, nullptr };
    }

    template<class CLASS, class TYPE>
    void set_property_value(CLASS* obj, const char* name, const TYPE& value)
    {
        auto prop = reflection::find_property<CLASS, TYPE>(obj, name);
        prop.set(value);
    }
}

#endif

