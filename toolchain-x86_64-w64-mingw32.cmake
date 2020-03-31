# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc-posix)
SET(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
SET(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
SET(CMAKE_AR x86_64-w64-mingw32-ar)
SET(CMAKE_RANLIB x86_64-w64-mingw32-ranlib)

# target environment location
SET(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32 /home/john/GIT/build-win64/8.2/inst)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(WIN32 ON)
unset(LINUX)
unset(UNIX)
unset(APPLE)