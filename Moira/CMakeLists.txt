add_library(moira STATIC

Moira.cpp
MoiraDebugger.cpp
)

target_compile_options(moira PUBLIC -Wno-unused-parameter)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(moira PUBLIC -fconcepts)
endif()
