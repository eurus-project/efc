set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/bin/g++)

set(CMAKE_C_LINKER /usr/bin/ld)
set(CMAKE_CXX_LINKER /usr/bin/ld)

set(CMAKE_ASM_COMPILER /usr/bin/as)

set(CMAKE_AR /usr/bin/ar)


set(CMAKE_RANLIB /usr/bin/ranlib)
set(CMAKE_FIND_LIBRARY_PREFIXES "")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")

set(CMAKE_LIBRARY_PATH /lib /usr/lib /usr/local/lib)

set(ENV{LIBGCC} "")
set(ENV{LIBC} "")
set(ENV{LIBM} "")
