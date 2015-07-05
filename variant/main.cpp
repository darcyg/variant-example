#include <type_traits>
#include <typeinfo>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <memory>


namespace ax {

    struct value;

    namespace detail {
       
        struct icontainer {
            virtual ~icontainer () = default;
            virtual const std::type_info& type () const = 0;
            virtual const void* ptr () const = 0;
            virtual void* ptr () = 0;
            virtual void copy (void* to) const = 0;
            virtual void move (void* to) = 0;
        };

        template <typename _Ptype>
        struct primitive_container: icontainer {
            primitive_container (_Ptype v) :_value (v) {}           
            const std::type_info& type () const override { return typeid (_Ptype); }
            const void* ptr () const override { return &_value; }
            void* ptr () override { return &_value; }
            void copy (void* to) const override {
                new (to) primitive_container<_Ptype> (_value);
            }
            void move (void* to) override {
                new (to) primitive_container<_Ptype> (_value);
            }
        private:
            _Ptype _value;
        };

        template <typename _Ctype>
        struct indirect_container: icontainer {

            indirect_container (const _Ctype& v)
                :_value (std::make_unique<_Ctype> (v))
            {}

            indirect_container (_Ctype&& v)
                :_value (std::make_unique<_Ctype> (std::move (v))) 
            {}

            indirect_container (indirect_container&& c)
                :_value (std::move (c._value))
            {}

            const std::type_info& type () const override { return typeid (_Ctype); }
            const void* ptr () const override { return _value.get (); }
            void* ptr () override { return _value.get (); }
            void copy (void* to) const override {
                new (to) indirect_container<_Ctype> (*_value);
            }
            void move (void* to) override {
                new (to) indirect_container<_Ctype> (std::move (*this));
            }
        protected:
            std::unique_ptr<_Ctype> _value;
        };

        
    }

    


    struct value {
        struct exception: std::exception {
            exception (const std::string& v): msg (v) {}
            const char* what () noexcept { return msg.c_str (); }
        private:
            std::string msg;
        };

        struct bad_cast: exception {
            using exception::exception;
        };

        typedef std::vector<value> vector_type;
        typedef std::map<std::string, value> map_type;
        typedef std::string string_type;

        value () : value (nullptr) {}
        value (std::nullptr_t v)        { new (&_container) detail::primitive_container<std::nullptr_t> (v); }
        value (bool v)                  { new (&_container) detail::primitive_container<std::uint64_t> (v ? 1 : 0); }
        value (std::uint64_t v)         { new (&_container) detail::primitive_container<std::uint64_t> (v); }
        value (std::int64_t v)          { new (&_container) detail::primitive_container<std::int64_t> (v); }
        value (std::uint32_t v)         { new (&_container) detail::primitive_container<std::uint64_t> (v); }
        value (std::int32_t v)          { new (&_container) detail::primitive_container<std::int64_t> (v); }
        value (std::uint16_t v)         { new (&_container) detail::primitive_container<std::uint64_t> (v); }
        value (std::int16_t v)          { new (&_container) detail::primitive_container<std::int64_t> (v); }
        value (std::uint8_t v)          { new (&_container) detail::primitive_container<std::uint64_t> (v); }
        value (std::int8_t v)           { new (&_container) detail::primitive_container<std::int64_t> (v); }
        value (double v)                { new (&_container) detail::primitive_container<double> (v); }
        value (float v)                 { new (&_container) detail::primitive_container<float> (v); }
        value (const char* v)           { new (&_container) detail::indirect_container<string_type> (v); }
        value (const std::string& v)    { new (&_container) detail::indirect_container<string_type> (v); }

        explicit value (const std::initializer_list<vector_type::value_type>& v) {
            new (&_container) detail::indirect_container<vector_type> (v);
        }

        explicit value (const std::initializer_list<map_type::value_type>& v) {
            new (&_container) detail::indirect_container<map_type> (v);
        }

        value (const value& v) {
            v.container ().copy (&_container);
        }

        value (value&& v) {
            v.container ().move (&_container);
        }

        ~value () {
            container ().~icontainer ();
        }

        value& operator = (const value& v) {
            container ().~icontainer ();
            v.container ().copy (&_container);
            return *this;
        }

        value& operator = (value&& v) {
            container ().~icontainer ();
            v.container ().move (&_container);
            return *this;
        }


        template <typename _Ttype>
        const _Ttype& get () const {            
            if (typeid (_Ttype) == container ().type ())
                return *reinterpret_cast<const _Ttype*> (container ().ptr ());
            throw bad_cast ("Bad type conversion");
        }

    private:
        const detail::icontainer& container () const {
            return reinterpret_cast<const detail::icontainer&> (_container);
        }
        detail::icontainer& container () {
            return reinterpret_cast<detail::icontainer&> (_container);
        }

        typedef std::aligned_union_t<16u,
            detail::primitive_container<std::nullptr_t>,
            detail::primitive_container<std::uint64_t>,
            detail::primitive_container<std::int64_t>,
            detail::primitive_container<double>,
            detail::indirect_container<std::string>,
            detail::indirect_container<std::vector<value>>,
            detail::indirect_container<std::map<std::string, value>>
        > container_store;
        
        container_store _container;
    };

    typedef value object, array;
    
    //struct object: value {};    
    //struct array: value {};

    template <typename _Vtype>
    const _Vtype& cast (const value& v) {
        return v.get<_Vtype> ();
    }
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

    auto v = ax::cast<std::int64_t> (v4);
    auto o = ax::cast<ax::value::map_type> (v17).at ("key0");


    return 0;
}
