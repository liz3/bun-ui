project(bun-ui)
set(CMAKE_C_STANDARD 11)

add_library(bun-ui SHARED src/la.c src/bun-ui.c src/glad.c)
add_subdirectory(third-party/glfw)
target_include_directories(bun-ui PRIVATE third-party/glfw/include)
target_link_libraries(bun-ui PUBLIC glfw)
