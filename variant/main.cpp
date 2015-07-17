#include <iostream>
#include <type_traits>
#include <typeinfo>
#include <map>
#include <string>
#include <vector>
#include <memory>

namespace ax {
    namespace detail {
        template <typename _Stype>
        _Stype repeat_string (const _Stype& s, unsigned n) {
            _Stype tmp;
            for (auto i = 0u; i < n; ++i) {
                tmp.append (s);
            }
            return tmp;
        }

        template <typename _Stype>
        _Stype serialize_string (const _Stype& s) {
            _Stype tmp;
            for (const auto& c : s) {
                switch (c) {
                case '"':
                    tmp.push_back('\\');
                    tmp.push_back('"');
                    continue;
                case '\'':
                    tmp.push_back('\\');
                    tmp.push_back('\'');
                    continue;
                case '\\':
                    tmp.push_back('\\');
                    tmp.push_back('\\');
                    continue;
                case '\n':
                    tmp.push_back('\\');
                    tmp.push_back('n');
                    continue;
                case '\r':
                    tmp.push_back('\\');
                    tmp.push_back('r');
                    continue;
                case '\t':
                    tmp.push_back('\\');
                    tmp.push_back('t');
                    continue;
                case '\a':
                    tmp.push_back('\\');
                    tmp.push_back('a');
                    continue;
                case '\b':
                    tmp.push_back('\\');
                    tmp.push_back('b');
                    continue;
                case '\f':
                    tmp.push_back('\\');
                    tmp.push_back('f');
                    continue;
                case '\v':
                    tmp.push_back('\\');
                    tmp.push_back('v');
                    continue;
                }
                if (c < 0x20 || c >= 0x7e) {
                    tmp.push_back('\\');
                    tmp.append(std::to_string(unsigned(c)));
                    continue;
                }
                tmp.push_back(c);
            }
            return "\"" + tmp + "\"";
        }

        template <typename _Stype, typename _Vtype>
        auto implode (const _Vtype& in, const _Stype& sep) {
            if (in.size () < 1u)
                return _Stype ();
            _Stype tmp;
            for (auto i = in.begin (); i != in.begin () + in.size () - 1u; ++i) {
                tmp.append (*i);
                tmp.append (sep);
            }
            tmp.append (*in.rbegin ());
            return tmp;
        }


        template <typename _Vtype>
        struct basic_container {
            typedef typename _Vtype::string_type string_type;
            typedef typename _Vtype::vector_type vector_type;
            typedef typename _Vtype::object_type object_type;

            typedef typename _Vtype::exception exception;
            typedef typename _Vtype::bad_cast bad_cast;
            typedef typename _Vtype::bad_index bad_index;

            virtual ~basic_container () = default;
            virtual const std::type_info& type () const = 0;
            virtual void copy (void* place) const = 0;
            virtual void move (void* place) = 0;

            virtual std::uint64_t   as_uint64   () const { throw bad_cast (type ().name (), typeid (std::uint64_t   ).name ()) ; }
            virtual std::int64_t    as_int64    () const { throw bad_cast (type ().name (), typeid (std::int64_t    ).name ()) ; }
            virtual double          as_double   () const { throw bad_cast (type ().name (), typeid (double          ).name ()) ; }
            virtual string_type     as_string   () const { throw bad_cast (type ().name (), typeid (string_type     ).name ()) ; }
            virtual string_type     serialize   (unsigned tab_size = 2u, unsigned depth  = 0u) const { return as_string () ; }

            virtual const _Vtype&   index       (const string_type& i) const { throw bad_index (type ().name (), i) ; }
            virtual _Vtype&         index       (const string_type& i)       { throw bad_index (type ().name (), i) ; }
            virtual const _Vtype&   index       (const std::size_t& i) const { throw bad_index (type ().name (), i) ; }
            virtual _Vtype&         index       (const std::size_t& i)       { throw bad_index (type ().name (), i) ; }

        };

        template <typename _Vtype, typename _Ctype>
        struct primitive_container: 
            basic_container<_Vtype> 
        {
            primitive_container (const _Ctype& init): value (init) {}
            const std::type_info& type () const override { return typeid (_Ctype); }
            void copy (void* place) const override { new (place) detail::primitive_container<_Vtype, _Ctype> (*this); }; // copying does not modify the original
            void move (void* place)       override { new (place) detail::primitive_container<_Vtype, _Ctype> (std::move (*this)); };

            std::uint64_t   as_uint64   () const override { return static_cast<std::uint64_t> (value) ; }
            std::int64_t    as_int64    () const override { return static_cast<std::int64_t> (value) ; }
            double          as_double   () const override { return static_cast<double> (value) ; }
            string_type     as_string   () const override { return std::to_string (value) ; }                        
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
            void copy (void* place) const override { new (place) detail::string_container<_Vtype> (*this); }; 
            void move (void* place)       override { new (place) detail::string_container<_Vtype> (std::move (*this)); };

            std::uint64_t   as_uint64   () const override { return std::stoull (*value) ; }
            std::int64_t    as_int64    () const override { return std::stoll (*value) ; }
            double          as_double   () const override { return std::stod (*value) ; }
            string_type     as_string   () const override { return *value ; }

            string_type serialize (unsigned tab_size = 2u, unsigned depth = 0u) const override { 
                return detail::serialize_string (*value) ; 
            };
        };

        template <typename _Vtype>
        struct nullptr_container: basic_container<_Vtype> {
            const std::type_info& type () const override { return typeid (nullptr); }
            void copy (void* place) const override { new (place) detail::nullptr_container<_Vtype> (); }; 
            void move (void* place)       override { new (place) detail::nullptr_container<_Vtype> (); };
            string_type     as_string   () const override { return "null" ; }
        };

        template <typename _Vtype>
        struct vector_container : indirect_container<_Vtype, typename _Vtype::vector_type> {
            using indirect_container<_Vtype, typename _Vtype::vector_type>::indirect_container;
            void copy (void* place) const override { new (place) detail::vector_container<_Vtype> (*this); }; 
            void move (void* place)       override { new (place) detail::vector_container<_Vtype> (std::move (*this)); };

            string_type as_string () const override { return serialize () ; }
            string_type serialize (unsigned tab_size = 2u, unsigned indent = 0u) const override {
                string_type is = detail::repeat_string<std::string>(" ", tab_size);
                string_type tmp;
                std::vector<string_type> out;
                for (const auto& item : *value) {
                    tmp.append (detail::repeat_string(is, indent + 1u));
                    tmp.append (item.serialize (tab_size, indent + 1u));
                    out.push_back (tmp);
                    tmp.clear ();
                }
                return "[\n" + implode<std::string> (out, ",\n") + "\n" + detail::repeat_string (is, indent) + "]";
            }

            const _Vtype& index (const string_type& i) const override { 
                std::size_t p;
                auto idx = std::stoull (i, &p) ;
                if (p != 0) {
                    return index (idx) ;
                }
                throw bad_index (type ().name (), i);            
            }

            _Vtype& index (const string_type& i) override {
                std::size_t p;
                auto idx = std::stoull (i, &p) ;
                if (p != 0) {
                    return index (idx) ;
                }
                throw bad_index (type ().name (), i);
            }

            const _Vtype& index (const std::size_t& i) const override { 
                if (value->size () <= i) {
                    static const _Vtype _null;
                    return _null;
                }
                return value->at (i);
            }

            _Vtype& index (const std::size_t& i) override { 
                if (value->size () <= i) 
                    value->resize (i) ; 
                return value->at (i) ; 
            }

        };

        template <typename _Vtype>
        struct object_container : indirect_container<_Vtype, typename _Vtype::object_type> {
            using indirect_container<_Vtype, typename _Vtype::object_type>::indirect_container;
            void copy (void* place) const override { new (place) detail::object_container<_Vtype> (*this); }; 
            void move (void* place)       override { new (place) detail::object_container<_Vtype> (std::move (*this)); };

            string_type as_string () const override { return serialize () ; }
            string_type serialize (unsigned tab_size = 2u, unsigned indent = 0u) const override {
                string_type is = detail::repeat_string<std::string> (" ", tab_size);
                string_type tmp;
                std::vector<string_type> out;
                for (const auto& item : *value) {
                    tmp.append (detail::repeat_string (is, indent + 1u));
                    tmp.append (detail::serialize_string (item.first));
                    tmp.append (": ");
                    tmp.append (item.second.serialize (tab_size, indent + 1u));
                    out.push_back (tmp);
                    tmp.clear ();
                }
                
                return "{\n" + implode<std::string> (out, ",\n") + "\n" + detail::repeat_string (is, indent) + "}";
            }
            
            const _Vtype& index (const string_type& i) const override { 
                if (value->find (i) != value->end ()) {
                    return value->at (i) ;
                }
                static const _Vtype _null;
                return _null;
            }

            _Vtype& index (const string_type& i) override { 
                return (*value) [i];
            }

            const _Vtype& index (const std::size_t& i) const override { 
                return index (std::to_string (i)) ;
            }

            _Vtype& index (const std::size_t& i) override { 
                return index (std::to_string (i)) ;
            }
        };

        template <typename _Vtype>
        using is_uint = std::enable_if_t<
            std::is_integral<_Vtype>::value &&
            std::is_unsigned<_Vtype>::value &&
            !std::is_same<_Vtype, bool>::value,
            int>;

        template <typename _Vtype>
        using is_bool = std::enable_if_t<
            std::is_same<_Vtype, bool>::value,
            int>;

        template <typename _Vtype>
        using is_int = std::enable_if_t<
            std::is_integral<_Vtype>::value &&
            std::is_signed<_Vtype>::value,
            int>;

        template <typename _Vtype>
        using is_float = std::enable_if_t<
            std::is_floating_point<_Vtype>::value,
            int>;

        template <typename _Vtype, typename _Value>
        using is_string = std::enable_if_t<
            std::is_same<_Vtype, typename _Value::string_type>::value,
            int>;

        template <typename _Vtype, typename _Value>
        using is_vector = std::enable_if_t<
            std::is_same<_Vtype, typename _Value::vector_type>::value,
            int>;

        template <typename _Vtype, typename _Value>
        using is_object = std::enable_if_t<
            std::is_same<_Vtype, typename _Value::object_type>::value,
            int>;

        template <typename _Vtype>
        using is_null = std::enable_if_t<
            std::is_same<_Vtype, std::nullptr_t>::value,
            int>;


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

        struct bad_index: exception {
            bad_index (const std::string& type, const std::string& key):
                exception ("Can't index value of type `"+type+"` or bad key `"+key+"`")
            {}
            bad_index (const std::string& type, const std::size_t& key):
                exception ("Can't index value of type `"+type+"` or bad key `"+std::to_string (key)+"`")
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

        bool is_null    () const { return container ().type () == typeid (std::nullptr_t); }
        bool is_object  () const { return container ().type () == typeid (object_type   ); }
        bool is_vector  () const { return container ().type () == typeid (vector_type   ); }
        bool is_string  () const { return container ().type () == typeid (string_type   ); }
        bool is_integer () const { return container ().type () == typeid (std::int64_t  ); }
        bool is_usigned () const { return container ().type () == typeid (std::uint64_t ); }
        bool is_double  () const { return container ().type () == typeid (double        ); }
        const std::type_info& type () const { return container ().type (); }        

        template <typename _Vtype, detail::is_uint<_Vtype> = 0>
        static _Vtype cast (const value& v) {
            return static_cast<_Vtype> (
                v.container ().as_uint64 ());
        }

        template <typename _Vtype, detail::is_int<_Vtype> = 0>
        static _Vtype cast(const value& v) {
            return static_cast<_Vtype> (
                v.container().as_int64());
        }

        template <typename _Vtype, detail::is_bool<_Vtype> = 0>
        static _Vtype cast (const value& v) {
            return v.container().as_uint64 () != 0;
        }

        template <typename _Vtype, detail::is_float<_Vtype> = 0>
        static _Vtype cast (const value& v) {
            return static_cast<_Vtype> (
                v.container ().as_double ());
        }

        template <typename _Vtype, detail::is_string<_Vtype, value> = 0>
        static _Vtype cast (const value& v) {
            return static_cast<_Vtype> (
                v.container ().as_string ());
        }

        string_type serialize (unsigned tab_size = 2u, unsigned indent = 0u) const {
            return container ().serialize (tab_size, indent);
        }        

              value& operator [] (const string_type& i)       { return container ().index (i) ; }
        const value& operator [] (const string_type& i) const { return container ().index (i) ; }
              value& operator [] (const std::size_t& i)       { return container ().index (i) ; }
        const value& operator [] (const std::size_t& i) const { return container ().index (i) ; }

        container_store value_store;
    };

    // typedef value object, vector; 
    // defininit these aliases for readability's sake
    struct object: value {
        using value::value;
        object () : value (std::initializer_list<vector_type::value_type> ()) {}
    };
    
    struct vector: value {
        using value::value;
        vector () : value (std::initializer_list<object_type::value_type> ()) {}
    };
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
                {"key0", ax::vector {111, 112, 113}},
                {"key1", ax::vector {111, 112, 113}},
                {"key2", ax::vector {111, 112, 113}},
            }},
            {"key1", ax::object {
                {"key0", ax::vector {121, 122, 123}},
                {"key1", ax::vector {121, 122, 123}},
                {"key2", ax::vector {121, 122, 123}},
            }}
        }},
        {"key1", ax::object {
            {"key0", ax::object {
                {"key0", ax::vector {211, 212, 213}},
                {"key1", ax::vector {211, 212, 213}},
                {"key2", ax::vector {211, 212, 213}},
            }},
            {"key1", ax::object {
                {"key0", ax::vector {221, 222, 223}},
                {"key1", ax::vector {221, 222, 223}},
                {"key2", ax::vector {221, 222, 223}},
            }}
        }}
    };

    std::cout << v17 ["key0"] ["key0"] ["key0"] [0].serialize () << "\n";
    std::cout << v17 ["key0"] ["key1"] ["key0"] ["1"].serialize () << "\n";
    std::cout << v17 ["key0"] ["key0"] ["key1"] ["2"].serialize () << "\n";


    auto score = 0u;

    score += (ax::value::cast<std::string>      (v0)    == "null");
    score += (ax::value::cast<std::string>      (v1)    == "null");
    score += (ax::value::cast<bool>             (v2)    == true);
    score += (ax::value::cast<char>             (v3)    == 'a');
    score += (ax::value::cast<std::int32_t >    (v4)    == -1);
    score += (ax::value::cast<std::uint32_t>    (v5)    == 1u);
    score += (ax::value::cast<std::int64_t >    (v6)    == -1ll);
    score += (ax::value::cast<std::uint64_t>    (v7)    == 1ull);
    score += (ax::value::cast<float>            (v8)    == 1.0f);
    score += (ax::value::cast<double>           (v9)    == 0.5);
    score += (ax::value::cast<std::string>      (v10)   == "A String Literal");
    score += (ax::value::cast<std::string>      (v11)   == "A std::string");

    score += (ax::value::cast<std::string>      (v12)   == "object (0)");
    score += (ax::value::cast<std::string>      (v13)   == "vector (0)");
    score += (ax::value::cast<std::string>      (v14)   == "vector (10)");
    score += (ax::value::cast<std::string>      (v15)   == "object (10)");
    score += (ax::value::cast<std::string>      (v16)   == "vector (2)");
    score += (ax::value::cast<std::string>      (v17)   == "object (2)");

    std::cout << score << "/18\n";

    return 0;
}
catch (const ax::value::exception& e) {
    std::cout << e.what () << "\n";
    return -1;
}