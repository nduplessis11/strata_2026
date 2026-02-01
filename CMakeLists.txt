cmake_minimum_required(VERSION 3.20)

project(strata_2026 VERSION 0.1.0 LANGUAGES CXX)

option(STRATA_USE_GNU_EXTENSIONS "Use GNU extensions (-std=gnu++23) instead of ISO (-std=c++23)" ON)
option(STRATA_WARNINGS "Enable extra compiler warnings" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ${STRATA_USE_GNU_EXTENSIONS})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Header-only target for include/
add_library(strata_headers INTERFACE)
target_include_directories(strata_headers INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>

)
target_compile_features(strata_headers INTERFACE cxx_std_23)

if (STRATA_WARNINGS)
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(strata_headers INTERFACE -Wall -Wextra -Wpedantic)

  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

    target_compile_options(strata_headers INTERFACE /W4)
  endif()

endif()

add_executable(strata_demo
  src/main.cpp

)

# ---- Copy MinGW runtime DLLs next to the exe
option(STRATA_COPY_MINGW_RUNTIME_DLLS "Copy MinGW runtime DLLs next to built executables" ON)

if(MINGW AND STRATA_COPY_MINGW_RUNTIME_DLLS)
  if(WIN32)
    # Windows-host MinGW: find runtime DLLs on PATH
    find_file(_dll_libstdcxx NAMES libstdc++-6.dll PATHS ENV PATH NO_DEFAULT_PATH)
    find_file(_dll_libgcc    NAMES libgcc_s_seh-1.dll PATHS ENV PATH NO_DEFAULT_PATH)
    find_file(_dll_pthread   NAMES libwinpthread-1.dll PATHS ENV PATH NO_DEFAULT_PATH)


    if(NOT _dll_libstdcxx OR NOT _dll_libgcc OR NOT _dll_pthread)
      message(FATAL_ERROR
        "Could not find MinGW runtime DLLs on PATH.\n"
        "Ensure your MinGW bin folder is on PATH and contains:\n"
        "  libstdc++-6.dll\n  libgcc_s_seh-1.dll\n  libwinpthread-1.dll")
    endif()

    set(_dlls_full "${_dll_libstdcxx}" "${_dll_libgcc}" "${_dll_pthread}")
  else()
    # WSL/Linux-host MinGW: use Arch sysroot

    set(_dll_dir "/usr/x86_64-w64-mingw32/bin")
    set(_dlls_full
      "${_dll_dir}/libstdc++-6.dll"
      "${_dll_dir}/libgcc_s_seh-1.dll"
      "${_dll_dir}/libwinpthread-1.dll"
    )
    foreach(_p IN LISTS _dlls_full)
      if(NOT EXISTS "${_p}")
        message(FATAL_ERROR "Missing MinGW runtime DLL: ${_p}")
      endif()
    endforeach()
  endif()


  foreach(_dll_path IN LISTS _dlls_full)
    add_custom_command(TARGET strata_demo POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${_dll_path}"
              "$<TARGET_FILE_DIR:strata_demo>"
      VERBATIM
    )
  endforeach()
endif()

target_link_libraries(strata_demo PRIVATE strata_headers)

# Better symbolization for stack traces on Linux
if (UNIX AND NOT APPLE)
  target_link_options(strata_demo PRIVATE -rdynamic)
endif()

# libstdc++: std::stacktrace symbols may live in libstdc++exp (GCC 14+)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14)

    message(FATAL_ERROR "GCC 14+ is required for this project (uses <print>/std::println and std::stacktrace).")
  endif()

  # Prefer absolute path if found, but fall back to -lstdc++exp
  find_library(LIBSTDCXXEXP NAMES stdc++exp)
  if (LIBSTDCXXEXP)
    target_link_libraries(strata_demo PRIVATE ${LIBSTDCXXEXP})
  else()
    target_link_libraries(strata_demo PRIVATE stdc++exp)

  endif()
endif()

