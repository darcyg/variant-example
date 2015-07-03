#include <type_traits>
#include <typeinfo>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>


namespace ax {

    struct value {
        enum {
            tag_null,
            tag_bool,
            tag_uint8,
            tag_int8,
            tag_uint16,
            tag_int16,
            tag_uint32,
            tag_int32,
            tag_uint64,
            tag_int64,
            tag_char8,
            tag_wchar,
            tag_char16,
            tag_char32,
            tag_float,
            tag_double,
            tag_str8,
            tag_wstr,
            tag_str16,
            tag_str32,
            tag_array,
            tag_object
        };

        typedef std::map<std::string, value> object_t;
        typedef std::vector<value> array_t;

        value (): value (nullptr)  {}
        value (std::nullptr_t in)  :tag (tag_null),     data (reinterpret_cast<std::uintptr_t&>(in)) {}
        value (bool in)            :tag (tag_bool),     data (reinterpret_cast<std::uint8_t&>(in)) {}        
        value (std::uint8_t in)    :tag (tag_uint8),    data (reinterpret_cast<std::uint8_t&>(in)) {}
        value (std::int8_t in)     :tag (tag_int8),     data (reinterpret_cast<std::uint8_t&>(in)) {}
        value (std::uint16_t in)   :tag (tag_uint16),   data (reinterpret_cast<std::uint16_t&>(in)) {}
        value (std::int16_t in)    :tag (tag_int16),    data (reinterpret_cast<std::uint16_t&>(in)) {}
        value (std::uint32_t in)   :tag (tag_uint32),   data (reinterpret_cast<std::uint32_t&>(in)) {}
        value (std::int32_t in)    :tag (tag_int32),    data (reinterpret_cast<std::uint32_t&>(in)) {}
        value (std::uint64_t in)   :tag (tag_uint64),   data (reinterpret_cast<std::uint64_t&>(in)) {}
        value (std::int64_t in)    :tag (tag_int64),    data (reinterpret_cast<std::uint64_t&>(in)) {}
        value (char in)            :tag (tag_char8),    data (reinterpret_cast<std::uint64_t&>(in)) {}
        value (wchar_t in)         :tag (tag_wchar),    data (reinterpret_cast<std::uint64_t&>(in)) {}
        value (char16_t in)        :tag (tag_char16),   data (reinterpret_cast<std::uint64_t&>(in)) {}
        value (char32_t in)        :tag (tag_char32),   data (reinterpret_cast<std::uint64_t&>(in)) {}
        value (float in)           :tag (tag_float),    data (reinterpret_cast<std::uint32_t&>(in)) {}
        value (double in)          :tag (tag_double),   data (reinterpret_cast<std::uint64_t&>(in)) {}
        value (std::string in)     :tag (tag_str8),     data (reinterpret_cast<std::uintptr_t>(new std::string    (std::move (in)))) {}
        value (std::wstring in)    :tag (tag_wstr),     data (reinterpret_cast<std::uintptr_t>(new std::wstring   (std::move (in)))) {}
        value (std::u16string in)  :tag (tag_str16),    data (reinterpret_cast<std::uintptr_t>(new std::u16string (std::move (in)))) {}
        value (std::u32string in)  :tag (tag_str32),    data (reinterpret_cast<std::uintptr_t>(new std::u32string (std::move (in)))) {}        
        value (const char* in)     :value (std::string    (in)) {}
        value (const wchar_t* in)  :value (std::wstring   (in)) {}
        value (const char16_t* in) :value (std::u16string (in)) {}
        value (const char32_t* in) :value (std::u32string (in)) {}

        explicit value (const std::initializer_list<array_t::value_type>& in) 
            :tag (tag_array), data (reinterpret_cast<std::uint64_t> (
                new array_t (in.begin (), in.end ())))
        {}

        explicit value (const std::initializer_list<object_t::value_type>& in)
            :tag (tag_object), data (reinterpret_cast<std::uint64_t> (
                new object_t (in.begin (), in.end ())))
        {}
        

    private:
        template <typename _Ttype>
        auto& ref () {
            return reinterpret_cast<_Ttype&> (data);
        }

        union {
            std::uint64_t data;
            array_t *pArray;
            object_t *pObject;
        };
        int tag;
    };

    typedef value object;
    typedef value array;

    //struct object: value {
    //    
    //};
    //
    //struct array: value {
    //
    //};
}


int main () {
    ax::value v0;                                   // default initialization
    ax::value v1 = nullptr;                         // nullptr
    ax::value v2 = true;                            // a boolean value
    ax::value v3 = 'a';                             // ASCII character (not going to support utf chars for the sake of brevity)
    ax::value v4 = -1;                              // 32bit int
    ax::value v5 = 1u;                              // 32bit unsigned int
    ax::value v6 = -1ll;                            // 64bit int
    ax::value v7 = 1ull;                            // 64bit unsigned int
    ax::value v8 = 1.0f;                            // 32bit float
    ax::value v9 = 0.5;                             // 64bit double
    ax::value v10 = "A String Literal";             // ASCII/UTF-8 string (not going to suppor UTF-16 or UTF-32 for the sake brevity)
    ax::value v11 = std::string ("A std::string");  // a std::string

    auto v12 = ax::array {};                        // An empty array
    auto v13 = ax::object {};                       // an empty object
    auto v14 = ax::array {                          // An array of values
        nullptr, true, 'a', 
       -1, 0xFFFFFFFFu, -1ll, 
        0xFFFFFFFFFFFFFFFFull, 
        1.0f, 1.0, "A string"
    }; 
    
    auto v15 = ax::object {                         // An object
        {"key0", nullptr},
        {"key1", true},
        {"key2", 'a'},
        {"key3", -1},
        {"key4", 0xffffffffu},
        {"key5", -1ll},
        {"key6", 0xffffffffffffffffu},
        {"key7", 1.0f},
        {"key8", 1.0},
        {"key9", "A string"}
    };

    auto v16 = ax::array {                          // array within object within array
        ax::object {
            {"key0", 1},
            {"key1", ax::array {
                2, 3.0f, 4.0, "5"
            }}
        },
        ax::object {
            {"key0", 1},
            {"key1", ax::array {
                2, 3.0f, 4.0, "5"
            }}
        }
    };

    auto v17 = ax::object {                         // nesting arrays within objects within objects within objects
        {"key0", ax::object {
            {"key0", ax::object {
                {"key0", ax::array {1, 2, 3}},
                {"key1", ax::array {1, 2, 3}},
                {"key3", ax::array {1, 2, 3}},
            }},
            {"key1", ax::object {
                {"key0", ax::array {1, 2, 3}},
                {"key1", ax::array {1, 2, 3}},
                {"key3", ax::array {1, 2, 3}},
            }}
        }},
        {"key1", ax::object {
            {"key0", ax::object {
                {"key0", ax::array {1, 2, 3}},
                {"key1", ax::array {1, 2, 3}},
                {"key3", ax::array {1, 2, 3}},
            }},
            {"key1", ax::object {
                {"key0", ax::array {1, 2, 3}},
                {"key1", ax::array {1, 2, 3}},
                {"key3", ax::array {1, 2, 3}},
            }}
        }}
    };



    return 0;
}
