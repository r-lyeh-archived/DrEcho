DrEcho :pill: <a href="https://travis-ci.org/r-lyeh/DrEcho"><img src="https://api.travis-ci.org/r-lyeh/DrEcho.svg?branch=master" align="right" /></a>
======

- Dr Echo to spice your terminal up. Written in C++11.
- Dr Echo is cross-platform.
- Dr Echo is zlib/libpng licensed.

### API

```c++
#include <iostream>
#include "drecho.hpp"

// default settings
const bool dr::log_timestamp = true;
const bool dr::log_branch = true;
const bool dr::log_branch_scope = true;
const bool dr::log_text = true;
const bool dr::log_errno = true;
const bool dr::log_location = true;

int main(void) {
    // hello world, classic
    std::cout << "classic hello world" << std::endl;

    // DrEcho spices up this
    dr::echo << "new hello world. notice the extra timestamp on the left" << std::endl;

    // DrEcho can also detect posix, OpenGL and win32 errors automatically
    errno = EAGAIN;
    dr::echo << "an error will be catched on this line" << std::endl;

    // Set up a few custom keywords to highlight
    dr::highlight( DR_YELLOW, { "warn", "warning" } );
    dr::highlight( DR_GREEN, { "info", "debug" } );
    dr::highlight( DR_CYAN, { "another", "branch" } );
    dr::echo << "this is another warning with a few debug keywords" << std::endl;

    // Spice up cout from now
    dr::capture( std::cout );
    std::cout << "this cout will be highlighted" << std::endl;

    // Showcase tree usage
    {
        dr::tab scope;
        std::cout << "this is branch #1" << std::endl;
        {
            dr::tab scope;
            std::cout << "this is branch #1.1" << std::endl;
            std::cout << "this is branch #1.2" << std::endl;
            {
                dr::tab scope;
                std::cout << "this is branch #1.2.1" << std::endl;
                std::cout << "this is branch #1.2.2" << std::endl;
            }
            std::cout << "this is branch #1.3" << std::endl;
        }
        std::cout << "this is branch #2" << std::endl;
    }

    // Release our spiced up cout
    dr::release( std::cout );
    std::cout << ">> back to ordinary world" << std::endl;
}
```

### Possible output

![image](https://raw.github.com/r-lyeh/depot/master/drecho.png)
