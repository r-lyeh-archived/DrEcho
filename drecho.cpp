// DrEcho spices your terminal up
// - rlyeh, zlib/libpng licensed.

// @todo optional DR_ASSERT on detected errors

// @todo .html logs, unconditionally
// @todo .flat logs on linux, with no ansi codes
// @todo .ansi logs on windows, and then provide tint.exe viewer in tools/

// @todo hotkeys filtering in runtime (ie, strike 'd'e'b'u'g' keys to filter lines with 'debug' keywords only)
// @todo clipboard filtering in runtime (ie, copy this 'debug' text to filter lines with 'debug' keywords only)
// @todo also negative logic ('error -debug' : lines with 'error' keyword that have no 'debug' keywords in it)

// -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8<

#include <math.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>

#if defined(__APPLE__)
#   include <OpenGL/gl.h>
#   include <OpenGL/glu.h>
#   include <OpenGL/glx.h>
#   define DR_GLCONTEXT() glXGetCurrentContext()
#elif defined(_WIN32)
#   pragma comment(lib, "opengl32.lib")
#   pragma comment(lib, "glu32.lib")
#   include <windows.h>
#   include <GL/gl.h>
#   include <GL/glu.h>
#   define DR_GLCONTEXT() wglGetCurrentContext()
#else
#   include <GL/gl.h>
#   include <GL/glu.h>
#   include <GL/glx.h>
#   define DR_GLCONTEXT() glXGetCurrentContext()
#endif

#ifdef _WIN32
#   include <windows.h>
#   include <io.h>
#   define $win(...) __VA_ARGS__
#   define $welse(...)
#else
#   include <unistd.h>
#   include <sys/ioctl.h>
#   define $win(...)
#   define $welse(...) __VA_ARGS__
#endif

#ifdef _MSC_VER
#   define $msvc(...)  __VA_ARGS__
#   define $melse(...)
#else
#   define $msvc(...)
#   define $melse(...)  __VA_ARGS__
#endif

#ifdef _OPENMP
#include <omp.h>
namespace dr {
    double clock() {
        return omp_get_wtime();
    }
    const double epoch = dr::clock();
}
#else
#include <chrono>
namespace dr {
    double clock() {
        static const auto epoch = std::chrono::steady_clock::now();
        return std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::steady_clock::now() - epoch ).count() / 1000.0;
    }
    const double epoch = dr::clock();
}
#endif

#define DR_CLOCK fmod( dr::clock() - dr::epoch, 10000. )
#define DR_CLOCKs "%08.3fs"

#define DR_QUOTE(...) #__VA_ARGS__

#include "drecho.hpp"
#undef echo
#undef $cout
#undef $cerr
#undef $clog

// -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8<

namespace
{
    typedef $win(WORD) $welse(const char*) PlatformColorCode;

    PlatformColorCode GetPlatformColorCode(int color) {
        switch (color) {
            // from http://en.wikipedia.org/wiki/ANSI_escape_code
            case DR_BLACK:       return $welse(NULL)  $win(0);
            case DR_RED:         return $welse("31")  $win(FOREGROUND_RED | FOREGROUND_INTENSITY);
            case DR_GREEN:       return $welse("32")  $win(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            case DR_YELLOW:      return $welse("33")  $win(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            case DR_BLUE:        return $welse("34")  $win(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            case DR_MAGENTA:     return $welse("35")  $win(FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY);
            case DR_CYAN:        return $welse("36")  $win(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            $win( default: case DR_DEFAULT: )
            case DR_WHITE:       return $welse("37")  $win(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);

            $welse( default: case DR_DEFAULT: )
            case DR_GRAY:        return $welse("90")  $win(FOREGROUND_INTENSITY);
            case DR_RED_ALT:     return $welse("91")  $win(FOREGROUND_RED);
            case DR_GREEN_ALT:   return $welse("92")  $win(FOREGROUND_GREEN);
            case DR_YELLOW_ALT:  return $welse("93")  $win(FOREGROUND_RED | FOREGROUND_GREEN);
            case DR_BLUE_ALT:    return $welse("94")  $win(FOREGROUND_BLUE);
            case DR_MAGENTA_ALT: return $welse("95")  $win(FOREGROUND_BLUE | FOREGROUND_RED);
            case DR_CYAN_ALT:    return $welse("96")  $win(FOREGROUND_BLUE | FOREGROUND_GREEN);
            case DR_WHITE_ALT:   return $welse("97")  $win(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
       }
    }

    template<typename T>
    std::string to_string( T number ) {
        std::stringstream ss;
        return ss << number ? ss.str() : std::string("0");
    }
}

namespace
{
    // excerpt from https://github.com/r-lyeh/apathy library following
    namespace apathy
    {
        std::deque<std::string> split( const std::string &str, char sep )
        {
            std::deque<std::string> tokens;
            tokens.push_back( std::string() );

            for( std::string::const_iterator it = str.begin(), end = str.end(); it != end; ++it )
            {
                if( *it == sep )
                {
                    tokens.push_back( std::string() + sep );
                    tokens.push_back( std::string() );
                }
                else
                {
                    tokens.back() += *it;
                }
            }

            return tokens;
        }

        class sbb : public std::streambuf
        {
            public:

            typedef void (*proc)( bool open, bool feed, bool close, const std::string &text );
            typedef std::set< proc > set;
            set cb;

            sbb()
            {}

            sbb( const sbb &other ) {
                operator=(other);
            }

            sbb &operator=( const sbb &other ) {
                if( this != &other ) {
                    cb = other.cb;
                }
                return *this;
            }

            sbb( void (*cbb)( bool, bool, bool, const std::string & ) ) {
                insert( cbb );
            }

            ~sbb() {
                clear();
            }

            void log( const std::string &line ) {
                if( !line.size() )
                    return;

                std::deque<std::string> lines = split( line, '\n' );

                for( set::iterator jt = cb.begin(), jend = cb.end(); jt != jend; ++jt )
                    for( std::deque<std::string>::iterator it = lines.begin(), end = lines.end(); it != end; ++it )
                    {
                        if( *it != "\n" )
                            (**jt)( false, false, false, *it );
                        else
                            (**jt)( false, true, false, std::string() );
                    }
            }

            virtual int_type overflow( int_type c = traits_type::eof() ) {
                return log( std::string() + (char)(c) ), 1;
            }

            virtual std::streamsize xsputn( const char *c_str, std::streamsize n ) {
                return log( std::string( c_str, (unsigned)n ) ), n;
            }

            void clear() {
                for( const auto &jt : cb ) {
                    (*jt)( false, false, true, std::string() );
                }
                cb.clear();
            }

            void insert( proc p ) {
                if( !p )
                    return;

                // make a dummy call to ensure any static object of this callback are deleted after ~sbb() call (RAII)
                p( 0, 0, 0, std::string() );
                p( true, false, false, std::string() );

                // insert into map
                cb.insert( p );
            }

            void erase( proc p ) {
                p( false, false, true, std::string() );
                cb.erase( p );
            }
        };

        struct captured_ostream {
            std::streambuf *copy = 0;
            sbb sb;
        };

        std::map< std::ostream *, captured_ostream > loggers;

        namespace ostream
        {
            void attach( std::ostream &_os, void (*custom_stream_callback)( bool open, bool feed, bool close, const std::string &line ) )
            {
                std::ostream *os = &_os;

                ( loggers[ os ] = loggers[ os ] );

                if( !loggers[ os ].copy )
                {
                    // capture ostream
                    loggers[ os ].copy = os->rdbuf( &loggers[ os ].sb );
                }

                loggers[ os ].sb.insert( custom_stream_callback );
            }

            void detach( std::ostream &_os, void (*custom_stream_callback)( bool open, bool feed, bool close, const std::string &line ) )
            {
                std::ostream *os = &_os;

                attach( _os, custom_stream_callback );

                loggers[ os ].sb.erase( custom_stream_callback );

                if( !loggers[ os ].sb.cb.size() )
                {
                    // release original stream
                    os->rdbuf( loggers[ os ].copy );
                }
            }

            void detach( std::ostream &_os )
            {
                std::ostream *os = &_os;

                ( loggers[ os ] = loggers[ os ] ).sb.clear();

                // release original stream
                os->rdbuf( loggers[ os ].copy );
            }

            std::ostream &make( void (*proc)( bool open, bool feed, bool close, const std::string &line ) )
            {
                static struct container
                {
                    std::map< void (*)( bool open, bool feed, bool close, const std::string &text ), sbb > map;
                    std::vector< std::ostream * > list;

                    container()
                    {}

                    ~container()
                    {
                        for( std::vector< std::ostream * >::const_iterator
                                it = list.begin(), end = list.end(); it != end; ++it )
                            delete *it;
                    }

                    std::ostream &insert( void (*proc)( bool open, bool feed, bool close, const std::string &text ) )
                    {
                        ( map[ proc ] = map[ proc ] ) = sbb(proc);

                        list.push_back( new std::ostream( &map[proc] ) );
                        return *list.back();
                    }
                } _;

                return _.insert( proc );
            }
        } // ns ::apathy::ostream
    } // ns ::apathy
} // ns ::


namespace dr
{
    int printf( int color, const char* fmt, ... ) {
        int num;
        va_list args;
        va_start(args, fmt);

        $win(
            const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

            CONSOLE_SCREEN_BUFFER_INFO buffer_info;
            GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
            const WORD previous_attr = buffer_info.wAttributes;

            fflush(stdout);
            SetConsoleTextAttribute(stdout_handle, GetPlatformColorCode(color));

            num = vprintf(fmt, args);

            fflush(stdout);
            SetConsoleTextAttribute(stdout_handle, previous_attr);
        )
        $welse(
            const char* color_code = GetPlatformColorCode(color);
            // 24-bit console ESC[ … 38;2;<r>;<g>;<b> … m Select RGB foreground color
            // 256-color console ESC[38;5;<fgcode>m
            // 0x00-0x07:  standard colors (as in ESC [ 30..37 m)
            // 0x08-0x0F:  high intensity colors (as in ESC [ 90..97 m)
            // 0x10-0xE7:  6*6*6=216 colors: 16 + 36*r + 6*g + b (0≤r,g,b≤5)
            // 0xE8-0xFF:  grayscale from black to white in 24 steps
            if (color_code) fprintf(stdout, "\033[0;3%sm", color_code);
            num = vprintf(fmt, args);
            ::printf("%s","\033[m");
        )

        va_end(args);
        return num;
    }

    int print( int color, const std::string &str ) {
        return dr::printf( color, "%s", str.c_str() );
    }
}

// -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8<

namespace dr {

    std::string &file() {
        static std::string st;
        return st;
    }
    std::string &prefix() {
        static std::string st;
        return st;
    }
    std::string &spent() {
        static std::string st;
        return st;
    }
    unsigned &color() {
        static unsigned st = 0;
        return st;
    }

    scope::scope() : clock(dr::clock()) {
        prefix().push_back(' ');
    }
    scope::~scope() {
        spent() = std::string("scoped for ") + to_string( dr::clock() - clock ) + "s";
        prefix().pop_back();
    }
}

// -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8<

namespace dr {
    void clear_errors() {
        errno = 0;
        $win( SetLastError( ERROR_SUCCESS ) );
        if( DR_GLCONTEXT() ) {
            do {} while( glGetError() != GL_NO_ERROR );
        }
    }
    std::string get_any_error() {
        std::string err, gl, os;

        if( errno ) {
            err.resize( 2048 );

            $msvc( strerror_s(&err[0], err.size(), errno) );
            $melse( strcpy(&err[0], strerror(errno) ) );

            err = std::string("(errno ") + to_string(errno) + ": " + err.c_str() + std::string(")");
        }

        if( DR_GLCONTEXT() ) {
            GLenum gl_code;
            do {
                gl_code = glGetError();
                if( gl_code != GL_NO_ERROR ) {
                    gl += std::string("(glerrno ") + to_string(gl_code) + ": " + std::string((const char *)gluErrorString(gl_code)) + ")";
                }
            } while( gl_code != GL_NO_ERROR );
        }

        $win(
            DWORD os_code = GetLastError();

            if( os_code ) {
                LPVOID lpMsgBuf;
                DWORD buflen = FormatMessage(
                   FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM |
                   FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   os_code,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPTSTR) &lpMsgBuf,
                   0, NULL
                );

                if( buflen > 2 ) {
                    std::string text( (const char *)lpMsgBuf, buflen - 2 ); // remove \r\n
                    os = std::string("(w32errno ") + to_string(os_code) + ": " + text + ")";
                }

                if( buflen ) {
                    LocalFree( lpMsgBuf );
                }
            }
        )

        return err + gl + os;
    }
}

// -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8<

namespace dr {

    namespace {
        std::set< std::ostream * > captured;
        std::map< std::string, DR_COLOR > vhighlights;
    }

    void logger( bool open, bool feed, bool close, const std::string &line );

    bool capture( std::ostream &os_ ) {
        std::ostream *os = &os_;
        if( os != &dr::echo ) {
            if( dr::captured.find(os) == dr::captured.end() ) {
                dr::captured.insert(os);
                apathy::ostream::attach( *os, dr::logger );
                return true;
            }
        }
        return false;
    }

    bool release( std::ostream &os_ ) {
        std::ostream *os = &os_;
        if( os != &dr::echo ) {
            if( dr::captured.find(os) != dr::captured.end() ) {
                dr::captured.erase(os);
                apathy::ostream::detach( *os );
                return true;
            }
        }
        return false;
    }

    std::string lowercase( std::string text ) {
        for( auto &ch : text ) {
            if( ch >= 'A' && ch <= 'Z' ) ch = ( ch - 'A' ) + 'a';
        }
        return text;
    }

    // tokenize_incl_separators
    std::vector< std::string > split( const std::string &string, const std::string &delimiters ) {
        std::string str;
        std::vector< std::string > tokens;
        for( auto &ch : string ) {
            if( delimiters.find_first_of( ch ) != std::string::npos ) {
                if( str.size() ) tokens.push_back( str ), str = "";
                tokens.push_back( std::string() + ch );
            } else str += ch;
        }
        return str.empty() ? tokens : ( tokens.push_back( str ), tokens );
    }

    void highlight( DR_COLOR color, const std::vector<std::string> &user_highlights ) {
        for( auto &highlight : user_highlights ) {
            dr::vhighlights[ lowercase(highlight) ] = color;
        }
    }

    std::vector<std::string> highlights( DR_COLOR color ) {
        std::vector<std::string> out;
        for( auto &hl : dr::vhighlights ) {
            if( hl.second == color ) out.push_back( hl.first );
        }
        return out;
    }

    std::string location( const std::string &func, const std::string &file, int line ) {
        dr::file() = std::string("(at ") + func + "() " + file + ":" + to_string(line) + ")";
        return std::string();
    }

    void logger( bool open, bool feed, bool close, const std::string &line )
    {
        static std::string cache;

        if( open )
        {}
        else
        if( close )
        {}
        else
        if( feed )
        {
            if( cache.empty() )
                return;

            static size_t num_errors = 0;
            std::string err = dr::get_any_error();
            // num lines to display in red
            if( !err.empty() ) num_errors += 1; //5

            if( dr::log_timestamp ) {
                dr::printf( DR_WHITE_ALT, DR_CLOCKs " ", DR_CLOCK );
            }

            std::string prefix = dr::prefix();
            static int prevlvl = 0;
            int lvl = prefix.size(), last = lvl - 1;
            bool pushes = (lvl > prevlvl), pops = (lvl < prevlvl), same = (lvl == prevlvl);
            if( dr::log_branch ) {
                dr::printf( DR_GRAY, "|" );
                for( int i = 0; i < prefix.size(); i ++ ) {
                    int color = ( DR_GRAY + 1 + i ) % DR_TOTAL_COLORS;
                    /**/ if( lvl < prevlvl ) {
                        dr::printf( color, i==last ? "/" : "|" );
                    }
                    else if( lvl > prevlvl ) {
                        dr::printf( color, i==last ? "\\" : "|" );
                    }
                    else {
                        dr::printf( color, i==last ? "|" : "|" );
                    }
                }
                prevlvl = lvl;
                dr::printf( DR_DEFAULT, " " );
            }

            if( dr::log_text ) {
                std::vector<std::string> tags = split(lowercase(cache), "!\"#~$%&/(){}[]|,;.:<>+-/*@'\"\t\n\\ ");
                for( auto &tag : tags ) {
                    auto find = dr::vhighlights.find(tag);
                    int color = ( find == dr::vhighlights.end() ? DR_DEFAULT : find->second );
                    dr::print( color, tag );
                }
            }

            if( dr::log_errno ) {
                dr::printf( num_errors ? DR_RED : DR_DEFAULT, " %s", err.c_str() );
            }

            if( dr::log_location ) {
                if( dr::file().size() ) {
                    dr::printf( DR_GRAY, " %s", dr::file().c_str() );
                    dr::file() = std::string();
                }
            }

            if( dr::log_branch_scope ) {
                if( pops ) {
                    dr::printf( DR_MAGENTA, " %s", spent().c_str() );
                    spent() = std::string();
                }
            }

            num_errors = 0;
            dr::clear_errors();

            fputs( "\n", stdout );

            cache = std::string();
        }
        else
        {
            cache += line;
        }
    }
}

// -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8< -- 8<

namespace dr {
    std::ostream &echo = apathy::ostream::make(dr::logger);
}

#undef $welse
#undef $win
