// DrEcho spices your terminal up
// - rlyeh, zlib/libpng licensed.

// @todo : see drecho.cpp

#pragma once

#include <string.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#define DRECHO_VERSION "1.0.0" // (2016/04/11): Initial semantic versioning adherence

// DR API

enum DR_COLOR {
    DR_RED,
    DR_GREEN,
    DR_YELLOW,
    DR_BLUE,
    DR_MAGENTA,
    DR_CYAN,
    DR_WHITE,
    DR_GRAY,
    DR_RED_ALT,
    DR_GREEN_ALT,
    DR_YELLOW_ALT,
    DR_BLUE_ALT,
    DR_MAGENTA_ALT,
    DR_CYAN_ALT,
    DR_WHITE_ALT,

    DR_NONE,
    DR_DEFAULT = DR_NONE,
    DR_TOTAL_COLORS = DR_NONE,

    DR_BLACK,

    DR_PURPLE = DR_MAGENTA,
    DR_PURPLE_ALT = DR_MAGENTA_ALT
};

namespace dr {

    // app-defined boolean settings (default: true) {
    extern const bool log_timestamp;
    extern const bool log_branch;
    extern const bool log_branch_scope;
    extern const bool log_text;
    extern const bool log_errno;
    extern const bool log_location;
    // }

    // api for high-level logging
    extern std::ostream &echo;
    bool capture( std::ostream &os = std::cout );
    bool release( std::ostream &os = std::cout );
    void highlight( DR_COLOR color, const std::vector<std::string> &highlights );
    std::vector<std::string> highlights( DR_COLOR color );

    // api for low-level printing
    int print( int color, const std::string &str );
    int printf( int color, const char *str, ... );

    // api for errors
    std::string get_any_error();
    void clear_errors();

    // api for time
    double clock();

    // api for scopes
    struct scope {
         scope(/*attribs: tab, time, color*/);
        ~scope();
        double clock;
    };

    using tab = scope;

    // helper classes and utilities
    struct concat : public std::stringstream {
        template <typename T> concat & operator,(const T & val) {
            *this << val;
            return *this;
        }
    };

    std::string location( const std::string &func, const std::string &file, int line );
}

// -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8<

#ifdef _MSC_VER
#define DR_LINE   __LINE__
#define DR_FUNC   __FUNCTION__
#define DR_FILE   __FILE__
#else
#define DR_LINE   __LINE__
#define DR_FUNC   __PRETTY_FUNCTION__
#define DR_FILE   (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

// API for macros
#if defined(NDEBUG) || defined(_NDEBUG)
#   define DR_LOG(...)
#   define DR_SCOPE(...)
#else
#   define DR_LOG(...)    do { dr::echo << ( dr::concat(), __VA_ARGS__ ).str() << std::endl;  } while(0)
#   define DR_SCOPE(...)  dr::scope dr_scope(__VA_ARGS__)
#   define echo  echo << dr::location(DR_FUNC,DR_FILE,DR_LINE)
#   define $cerr cerr << dr::location(DR_FUNC,DR_FILE,DR_LINE)
#   define $cout cout << dr::location(DR_FUNC,DR_FILE,DR_LINE)
#   define $clog clog << dr::location(DR_FUNC,DR_FILE,DR_LINE)
#endif
