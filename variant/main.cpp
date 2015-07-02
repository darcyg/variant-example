#include <type_traits>
#include <typeinfo>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>


namespace ax {

    struct value {


    };

    struct object: value {

    };

    struct array: value {

    };
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