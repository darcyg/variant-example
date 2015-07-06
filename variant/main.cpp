#include <type_traits>
#include <typeinfo>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <memory>


namespace ax {

    namespace detail {

        template <typename _Vtype>
        struct icontainer {
            typedef typename _Vtype::string_type string_type;
            typedef typename _Vtype::vector_type vector_type;
            typedef typename _Vtype::map_type map_type;

            virtual ~icontainer () = default;
            virtual const std::type_info& type () const = 0;
            virtual void copy (void*) const = 0;
            virtual void move (void*) = 0;

            virtual std::int64_t    as_sint64 () const { throw typename _Vtype::bad_cast (type (), typeid (std::int64_t)); }
            virtual std::uint64_t   as_uint64 () const { throw typename _Vtype::bad_cast (type (), typeid (std::uint64_t)); }
            virtual double          as_double () const { throw typename _Vtype::bad_cast (type (), typeid (double)); }
            virtual string_type     as_string () const { throw typename _Vtype::bad_cast (type (), typeid (string_type)); }

            virtual _Vtype&         index (const std::size_t& i)        { throw typename _Vtype::bad_index (type (), i); }
            virtual const _Vtype&   index (const std::size_t& i) const  { throw typename _Vtype::bad_index (type (), i); }
            virtual _Vtype&         index (const string_type& k)        { throw typename _Vtype::bad_key (type (), k); }
            virtual const _Vtype&   index (const string_type& k) const  { throw typename _Vtype::bad_key (type (), k); }
        };

        template <typename _Vtype>
        struct null_container: icontainer<_Vtype> {
            null_container () {}
            const std::type_info& type () const override { return typeid (nullptr); }
            void copy (void* to) const  override { new (to) null_container<value> (); }
            void move (void* to)        override { new (to) null_container<value> (); }
        };


        template <typename _Vtype, typename _Ptype>
        struct primitive_container: icontainer<_Vtype> {
            primitive_container (_Ptype v):_value (v) {}
            const std::type_info& type () const override { return typeid (_Ptype); }
            void copy (void* to) const  override { new (to) primitive_container<value, _Ptype> (_value); }
            void move (void* to)        override { new (to) primitive_container<value, _Ptype> (_value); }

            std::int64_t    as_sint64 () const override { return static_cast<std::int64_t> (_value); };
            std::uint64_t   as_uint64 () const override { return static_cast<std::uint64_t> (_value); };
            double          as_double () const override { return static_cast<double> (_value); };
            string_type     as_string () const override { return std::to_string (_value); };

        private:
            _Ptype _value;
        };

        template <typename _Vtype, typename _Ctype>
        struct indirect_container: icontainer<_Vtype> {

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

        protected:
            std::unique_ptr<_Ctype> _value;
        };

        template <typename _Vtype>
        struct map_container: indirect_container<
            _Vtype, typename _Vtype::map_type> 
        {
            using indirect_container<_Vtype, typename _Vtype::map_type>::indirect_container;
            _Vtype&         index (const std::size_t& i)        override { return index (std::to_string (i)) ; }
            const _Vtype&   index (const std::size_t& i) const  override { return index (std::to_string (i)) ; }
            _Vtype&         index (const string_type& k)        override { return (*_value) [k] ; }
            const _Vtype&   index (const string_type& k) const  override { return (*_value).at (k) ; }
            void            copy (void* to) const  override { new (to) map_container<value> (*_value); }
            void            move (void* to)        override { new (to) map_container<value> (std::move (*this)); }
        };

        template <typename _Vtype>
        struct vector_container: indirect_container<
            _Vtype, typename _Vtype::vector_type> 
        {
            using indirect_container<_Vtype, typename _Vtype::vector_type>::indirect_container;
            _Vtype&         index (const std::size_t& i)        override { if (i >= _value->size ()) _value->resize (i+1u) ; return (*_value) [i] ; }
            const _Vtype&   index (const std::size_t& i) const  override { return (*_value).at (i) ; }
            _Vtype&         index (const string_type& k)        override { return index (std::size_t (std::stoull (k))) ; }
            const _Vtype&   index (const string_type& k) const  override { return index (std::size_t (std::stoull (k))) ; }
            void            copy (void* to) const  override { new (to) vector_container<value> (*_value); }
            void            move (void* to)        override { new (to) vector_container<value> (std::move (*this)); }
        };


        
        template <typename _Vtype>
        struct string_container: 
            indirect_container<_Vtype, typename _Vtype::string_type> 
        {
            using indirect_container<_Vtype, typename _Vtype::string_type>
                ::indirect_container;
            std::int64_t    as_sint64 () const override { return std::stoll (*_value); }
            std::uint64_t   as_uint64 () const override { return std::stoull (*_value); }
            double          as_double () const override { return std::stod (*_value); }
            string_type     as_string () const override { return *_value; }
            void            copy (void* to) const  override { new (to) string_container<value> (*_value); }
            void            move (void* to)        override { new (to) string_container<value> (std::move (*this)); }

        };

    }

    struct value {
        struct exception: std::exception {
            exception (const std::string& v): msg (v) {}
            const char* what () const noexcept override { return msg.c_str (); }
        private:
            std::string msg;
        };

        struct bad_cast: exception {
            bad_cast (const std::type_info& from, const std::type_info& to)
                :bad_cast (from.name (), to.name ())
            {}
            bad_cast (const std::string& from, const std::string& to)
                :exception ("Bad cast from " + from + " to " + to)
            {}
        };

        struct bad_index: exception {
            bad_index (const std::string& type, const std::size_t& index)
                :exception ("Bad index `" + std::to_string (index) + "` or can't index " + type) 
            {}
            bad_index (const std::type_info& type, const std::size_t& index)
                :bad_index (type.name (), index)
            {}
        };

        struct bad_key: exception {
            bad_key (const std::string& type, const std::string& index)
                :exception ("Bad key `" + index + "` or can't index " + type) 
            {}
            bad_key (const std::type_info& type, const std::string& index)
                :bad_key (type.name (), index)
            {}
        };


        typedef std::vector<value> vector_type;
        typedef std::map<std::string, value> map_type;
        typedef std::string string_type;

        value () : value (nullptr) {}
        value (std::nullptr_t v)        { new (&_container) detail::null_container<value> (); }
        value (bool v)                  { new (&_container) detail::primitive_container<value, std::uint64_t> (v ? 1u : 0u); }
        value (std::uint64_t v)         { new (&_container) detail::primitive_container<value, std::uint64_t> (v); }
        value (std::int64_t v)          { new (&_container) detail::primitive_container<value, std::int64_t> (v); }
        value (std::uint32_t v)         { new (&_container) detail::primitive_container<value, std::uint64_t> (v); }
        value (std::int32_t v)          { new (&_container) detail::primitive_container<value, std::int64_t> (v); }
        value (std::uint16_t v)         { new (&_container) detail::primitive_container<value, std::uint64_t> (v); }
        value (std::int16_t v)          { new (&_container) detail::primitive_container<value, std::int64_t> (v); }
        value (std::uint8_t v)          { new (&_container) detail::primitive_container<value, std::uint64_t> (v); }
        value (std::int8_t v)           { new (&_container) detail::primitive_container<value, std::int64_t> (v); }
        value (double v)                { new (&_container) detail::primitive_container<value, double> (v); }
        value (float v)                 { new (&_container) detail::primitive_container<value, double> (v); }
        value (const char* v)           { new (&_container) detail::string_container<value> (v); }
        value (const std::string& v)    { new (&_container) detail::string_container<value> (v); }

        explicit value (const std::initializer_list<vector_type::value_type>& v) {
            new (&_container) detail::vector_container<value> (v);
        }

        explicit value (const std::initializer_list<map_type::value_type>& v) {
            new (&_container) detail::map_container<value> (v);
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

        bool is_null    () const { return container ().type () == typeid (nullptr); }
        bool is_vector  () const { return container ().type () == typeid (vector_type); }
        bool is_map     () const { return container ().type () == typeid (map_type); }
        bool is_double  () const { return container ().type () == typeid (double); }
        bool is_int64   () const { return container ().type () == typeid (std::int64_t); }
        bool is_uint64  () const { return container ().type () == typeid (std::uint64_t); }

        value&          operator [] (const std::size_t& i)          { return container ().index (i); }
        const value&    operator [] (const std::size_t& i) const    { return container ().index (i); }
        value&          operator [] (const string_type& i)          { return container ().index (i); }
        const value&    operator [] (const string_type& i) const    { return container ().index (i); }
    
        template <typename _Vtype, std::enable_if_t<std::is_integral<_Vtype>::value && std::is_signed<_Vtype>::value, int> = 0>
        static _Vtype cast (const value& val) {
            return static_cast<_Vtype> (val.container ().as_sint64 ());
        }
        template <typename _Vtype, std::enable_if_t<std::is_integral<_Vtype>::value && std::is_unsigned<_Vtype>::value || std::is_same<_Vtype, bool>::value, int> = 0>
        static _Vtype cast (const value& val) {
            return static_cast<_Vtype> (val.container ().as_uint64 ());
        }
        template <typename _Vtype, std::enable_if_t<std::is_floating_point<_Vtype>::value, int> = 0>
        static _Vtype cast (const value& val) {
            return static_cast<_Vtype> (val.container ().as_double ());
        }
        template <typename _Vtype, std::enable_if_t<std::is_same<_Vtype, std::string>::value || std::is_same<_Vtype, const char*>::value, int> = 0>
        static std::string cast (const value& val) {
            return static_cast<_Vtype> (val.container ().as_string ());
        }
    private:
        const detail::icontainer<value>& container () const {
            return reinterpret_cast<const detail::icontainer<value> &> (_container);
        }
        detail::icontainer<value>& container () {
            return reinterpret_cast<detail::icontainer<value> &> (_container);
        }

        typedef std::aligned_union_t<16u,
            detail::null_container<value>,
            detail::primitive_container<value, std::uint64_t>,
            detail::primitive_container<value, std::int64_t>,
            detail::primitive_container<value, double>,
            detail::string_container<value>,
            detail::vector_container<value>,
            detail::map_container<value>
        > container_store;
        
        container_store _container;
    };

    typedef value map, vector;
    
    //struct map: value {};    
    //struct vector: value {};

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

    auto v12 = ax::vector {};                        // An empty vector
    auto v13 = ax::map {};                       // an empty map


    auto v14 = ax::vector {                          // An vector of values
        nullptr, true, 'a', 
       -1, 0xFFFFFFFFu, -1ll, 
        0xFFFFFFFFFFFFFFFFull, 
        1.0f, 1.0, "A string"
    }; 
    
    auto v15 = ax::map {                         // An map
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
    
    auto v16 = ax::vector {                          // vector within map within vector
        ax::map {
            {"key0", 1},
            {"key1", ax::vector {
                2, 3.0f, 4.0, "5"
            }}
        },
        ax::map {
            {"key0", 1},
            {"key1", ax::vector {
                2, 3.0f, 4.0, "5"
            }}
        }
    };
    
    auto v17 = ax::map {                         // nesting arrays within objects within objects within objects
        {"key0", ax::map {
            {"key0", ax::map {
                {"key0", ax::vector {1, 2, 3}},
                {"key1", ax::vector {1, 2, 3}},
                {"key3", ax::vector {1, 2, 3}},
            }},
            {"key1", ax::map {
                {"key0", ax::vector {1, 2, 3}},
                {"key1", ax::vector {1, 2, 3}},
                {"key3", ax::vector {1, 2, 3}},
            }}
        }},
        {"key1", ax::map {
            {"key0", ax::map {
                {"key0", ax::vector {1, 2, 3}},
                {"key1", ax::vector {1, 2, 3}},
                {"key3", ax::vector {1, 2, 3}},
            }},
            {"key1", ax::map {
                {"key0", ax::vector {1, 2, 3}},
                {"key1", ax::vector {1, 2, 3}},
                {"key3", ax::vector {1, 2, 3}},
            }}
        }}
    };

    std::cout << ax::value::cast<std::int64_t> (v4) << "\n" ;
    //std::cout << ax::value::cast<std::string> (v17) << "\n" ;
    std::cout << ax::value::cast<int> (v17 ["key0"] ["key0"] ["key0"] [1]);



    return 0;
}
catch (const ax::value::exception& e) {
    std::cout << e.what () << "\n";
    return -1;
}