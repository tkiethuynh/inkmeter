set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# zig cc wrappers — musl libc, no kernel version restrictions
set(CMAKE_C_COMPILER   "/tmp/zig-arm-cc"  CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "/tmp/zig-arm-cxx" CACHE FILEPATH "" FORCE)
set(CMAKE_AR           "/tmp/zig-arm-ar"  CACHE FILEPATH "" FORCE)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
