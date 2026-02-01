# Cross-compile to Windows (x86_64) from Linux/WSL using MinGW-w64.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(MINGW_TRIPLE x86_64-w64-mingw32)

set(CMAKE_C_COMPILER   ${MINGW_TRIPLE}-gcc)
set(CMAKE_CXX_COMPILER ${MINGW_TRIPLE}-g++)

set(CMAKE_RC_COMPILER  ${MINGW_TRIPLE}-windres)

# Where the toolchain lives on Arch
set(CMAKE_FIND_ROOT_PATH /usr/${MINGW_TRIPLE})

# Search headers/libs in the target sysroot, but keep host tools findable.

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Optional: avoid "try_run" executing Windows binaries during configure
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
