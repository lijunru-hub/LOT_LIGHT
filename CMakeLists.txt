# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS components/led_strip
                         components/esp-rainmaker/examples/common
                         components/esp-insights/components
                         components/esp-rainmaker/components
                         )


add_compile_options(-fdiagnostics-color=always)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(LOT_LIGHT)
