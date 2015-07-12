#include <iostream>
#include <type_traits>
#include <typeinfo>
#include <map>
#include <string>
#include <vector>
#include <memory>

namespace ax {

    namespace detail {
        template <typename _Vtype>
        struct basic_container {
            typedef typename _Vtype::string_type string_type;
            typedef typename _Vtype::vector_type vector_type;
            typedef typename _Vtype::object_type object_type;

            virtual ~basic_container () = default;
            virtual const std::type_info& type () const = 0;
            virtual void copy (void* place) const = 0;
            virtual void move (void* place) = 0;
        };

        template <typename _Vtype, typename _Ctype>
        struct primitive_container: 
            basic_container<_Vtype> 
        {
            primitive_container (const _Ctype& init): value (init) {}
            const std::type_info& type () const override { return typeid (_Ctype); }
            void copy (void* place) const override { new (place) detail::primitive_container<_Vtype, _Ctype> (*this); }; // copying does not modify the original
            void move (void* place)       override { new (place) detail::primitive_container<_Vtype, _Ctype> (std::move (*this)); };
        protected:
            _Ctype value;
        };

        template <typename _Vtype, typename _Ctype>
        struct indirect_container: 
            basic_container<_Vtype>
        {            
            indirect_container (const _Ctype& init): value (std::make_unique<_Ctype> (init)) {}
            indirect_container (const indirect_container<_Vtype, _Ctype>& init): value (std::make_unique<_Ctype> (*init.value)) {}
            indirect_container (indirect_container<_Vtype, _Ctype>&& init): value (std::move (init.value)) {}

            const std::type_info& type () const override { return typeid (_Ctype); }
            void copy (void* place) const override { new (place) detail::indirect_container<_Vtype, _Ctype> (*this); }; // copying does not modify the original
            void move (void* place)       override { new (place) detail::indirect_container<_Vtype, _Ctype> (std::move (*this)); };
        protected:
            std::unique_ptr<_Ctype> value;
            
        };

        template <typename _Vtype>
        struct string_container: indirect_container<_Vtype, typename _Vtype::string_type> {
            using indirect_container<_Vtype, typename _Vtype::string_type>::indirect_container;
        };

        template <typename _Vtype>
        struct vector_container: indirect_container<_Vtype, typename _Vtype::vector_type> {
            using indirect_container<_Vtype, typename _Vtype::vector_type>::indirect_container;
        };

        template <typename _Vtype>
        struct object_container: indirect_container<_Vtype, typename _Vtype::object_type> {
            using indirect_container<_Vtype, typename _Vtype::object_type>::indirect_container;
        };

        template <typename _Vtype>
        struct nullptr_container: basic_container<_Vtype> {
            const std::type_info& type () const override { return typeid (nullptr); }
            void copy (void* place) const override { new (place) detail::nullptr_container<_Vtype> (); }; 
            void move (void* place)       override { new (place) detail::nullptr_container<_Vtype> (); };
        };

    }


    struct value {
        // base for exceptions thrown by value
        struct exception: std::exception {
            exception (const std::string& msg):
                msg (msg)
            {}
            const char* what () const override { 
                return msg.c_str (); 
            }
        private:
            std::string msg;
        };

        struct bad_cast: exception {
            bad_cast (const std::string& from, const std::string& to):
                exception ("Can't convert from `" + from + "` to `" + to + "`")
            {}
            bad_cast (const std::string& to): 
                exception ("Can't convert to `" + to + "`")
            {}
        };


        // some utility typedefs
        typedef std::string string_type;
        typedef std::vector<value> vector_type;
        typedef std::map<string_type, value> object_type;
        typedef std::aligned_union_t<16u, // minimum size of the data to allocate
            detail::nullptr_container<value>,
            detail::primitive_container<value, std::uint64_t>,
            detail::primitive_container<value, std::int64_t>,
            detail::primitive_container<value, double>,
            detail::string_container<value>,
            detail::vector_container<value>,
            detail::object_container<value>>
            container_store;

        value (std::nullptr_t init)     { new (&value_store) detail::nullptr_container<value> () ; }
        value (bool init)               { new (&value_store) detail::primitive_container<value, std::uint64_t> (init ? 1u : 0u) ; }
        value (char init)               { new (&value_store) detail::primitive_container<value, std::int64_t> (init) ; }
        value (std::int8_t init)        { new (&value_store) detail::primitive_container<value, std::int64_t> (init) ; }
        value (std::uint8_t init)       { new (&value_store) detail::primitive_container<value, std::uint64_t> (init) ; }
        value (std::int16_t init)       { new (&value_store) detail::primitive_container<value, std::int64_t> (init) ; }
        value (std::uint16_t init)      { new (&value_store) detail::primitive_container<value, std::uint64_t> (init) ; }
        value (std::int32_t init)       { new (&value_store) detail::primitive_container<value, std::int64_t> (init) ; }
        value (std::uint32_t init)      { new (&value_store) detail::primitive_container<value, std::uint64_t> (init) ; }
        value (std::int64_t init)       { new (&value_store) detail::primitive_container<value, std::int64_t> (init) ; }
        value (std::uint64_t init)      { new (&value_store) detail::primitive_container<value, std::uint64_t> (init) ; }
        value (float init)              { new (&value_store) detail::primitive_container<value, double> (init) ; }
        value (double init)             { new (&value_store) detail::primitive_container<value, double> (init) ; }
        value (const string_type& init) { new (&value_store) detail::string_container<value> (init) ; }
        value (const char* init)        { new (&value_store) detail::string_container<value> (init) ; }
        value (): value (nullptr) {}

        explicit value (std::initializer_list<vector_type::value_type> init) {
            new (&value_store) detail::vector_container<value> (init);
        }
        explicit value (std::initializer_list<object_type::value_type> init) {
            new (&value_store) detail::object_container<value> (init);
        }

              detail::basic_container<value>& container ()       { return *reinterpret_cast<detail::basic_container<value> *      > (&value_store); }
        const detail::basic_container<value>& container () const { return *reinterpret_cast<detail::basic_container<value> const *> (&value_store); }

        value (const value& init) {
            init.container ().copy (&value_store);
        }

        value (value&& init) {
            init.container ().move (&value_store);
        }

        container_store value_store;
    };

    typedef value object, vector; 
    // defininit these aliases for readability's sake

}


int main () try {


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

    auto v12 = ax::vector {};                       // An empty vector
    auto v13 = ax::object {};                          // an empty map


    auto v14 = ax::vector {                         // An vector of values
        nullptr, true, 'a', 
       -1, 0xFFFFFFFFu, -1ll, 
        0xFFFFFFFFFFFFFFFFull, 
        1.0f, 1.0, "A string"
    }; 
    
    auto v15 = ax::object {                            // An map
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
    
    auto v16 = ax::vector {                         // vector within map within vector
        ax::object {
            {"key0", 1},                            
            {"key1", ax::vector {
                2, 3.0f, 4.0, "5"
            }}
        },
        ax::object {
            {"key0", 1},
            {"key1", ax::vector {
                2, 3.0f, 4.0, "5"
            }}
        }
    };
    
    auto v17 = ax::object {                            // nesting arrays within objects within objects within objects
        {"key0", ax::object {
            {"key0", ax::object {
                {"key0", ax::vector {1, 2, 3}},
                {"key1", ax::vector {1, 2, 3}},
                {"key3", ax::vector {1, 2, 3}},
            }},
            {"key1", ax::object {
                {"key0", ax::vector {1, 2, 3}},
                {"key1", ax::vector {1, 2, 3}},
                {"key3", ax::vector {1, 2, 3}},
            }}
        }},
        {"key1", ax::object {
            {"key0", ax::object {
                {"key0", ax::vector {1, 2, 3}},
                {"key1", ax::vector {1, 2, 3}},
                {"key3", ax::vector {1, 2, 3}},
            }},
            {"key1", ax::object {
                {"key0", ax::vector {1, 2, 3}},
                {"key1", ax::vector {1, 2, 3}},
                {"key3", ax::vector {1, 2, 3}},
            }}
        }}
    };

    return 0;
}
catch (const ax::value::exception& e) {
    std::cout << e.what () << "\n";
    return -1;
}