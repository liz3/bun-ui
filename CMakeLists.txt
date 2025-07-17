project(bun-ui)
cmake_minimum_required(VERSION 3.6)

if(CMAKE_JS_VERSION)
    set(CMAKE_CXX_STANDARD 17)
    add_library(bun-ui SHARED src/la.c src/bun-ui.c src/glad.c src/node_api.cc ${CMAKE_JS_SRC})
else()
    set(CMAKE_C_STANDARD 11)
    add_library(bun-ui SHARED src/la.c src/bun-ui.c src/glad.c)
endif()
add_subdirectory(third-party/glfw)
target_include_directories(bun-ui PRIVATE third-party/glfw/include)
target_link_libraries(bun-ui PUBLIC glfw)
if(CMAKE_JS_VERSION)
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_JS_LIB})
    target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)


    # Include Node-API wrappers
    execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
    message("${NODE_ADDON_API_DIR}")
    string(REGEX REPLACE "[\r\n\"]" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

    target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})
    if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
      # Generate node.lib
      execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
    endif()

    # define NAPI_VERSION
    add_definitions(-DNAPI_VERSION=3)
    add_definitions(-DBUN_UI_NODE_INTEGRATION)
endif()