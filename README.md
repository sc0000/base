# Base

This is a general purpose codebase, or homemade standard library, in very early stages. So far, it consists entirely of a pretty common dynamic array and a bunch of (more interesting) mixed-type allocators, to ease the pain of manual memory management. There's also some basic logging capabilities.

## Build, Include, Link

* GCC: Run `make lib`, copy the newly created `./base` folder into your project directory, add the necessary compiler flags like this: `gcc -Ibase/include *.c -Lbase/lib -lbase -o my_executable`
* MSVC: Run `build.bat` from the VS command prompt, copy the newly created `./base` folder into your project directory, add the necessary compiler flags like this: `cl /Ibase\include *.c /link /LIBPATH:base\lib base.lib /OUT:my_executable.exe`
* Either way, include the headers like this: `#include <base/allocators.h>`.
* Start coding! 🚀
