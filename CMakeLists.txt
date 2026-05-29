cmake_minimum_required(VERSION 3.5)
project(ai-usage LANGUAGES C CXX)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

# ---- LVGL ----
set(LV_CONF_INCLUDE_SIMPLE OFF)
set(LV_CONF_PATH "${CMAKE_SOURCE_DIR}/lv_conf.h" CACHE STRING "" FORCE)

add_subdirectory(lvgl)

target_compile_definitions(lvgl PUBLIC
    LV_CONF_INCLUDE_SIMPLE=0
    LV_CONF_PATH="${CMAKE_SOURCE_DIR}/lv_conf.h"
)

# ---- App ----
add_executable(ai-usage
    src/main.cpp
    src/fetch.cpp
    src/ui.cpp
)

target_include_directories(ai-usage PRIVATE
    src
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(ai-usage PRIVATE lvgl)

# Static binary: avoids soft-float vs hard-float ABI mismatch on Kindle
target_link_options(ai-usage PRIVATE -static)
