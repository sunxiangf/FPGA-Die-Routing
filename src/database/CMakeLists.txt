add_library(libdb database.cpp dijkstra.cpp)
target_include_directories(libdb PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(libdb PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)