add_library(musashi STATIC

m68kfpu.cpp
m68kcpu.cpp
m68kdasm.cpp
m68kops.cpp
)

target_compile_options(musashi PUBLIC -Wno-strict-aliasing)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(musashi PUBLIC -Wno-format-overflow)
else()
  target_compile_options(musashi PUBLIC -Wno-string-concatenation)
endif()
