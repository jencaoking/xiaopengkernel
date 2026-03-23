cmake_minimum_required(VERSION 3.15)
project(xiaopengkernel VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_EXTENSIONS OFF)

set(CURL_ROOT "${CMAKE_SOURCE_DIR}/third_party/curl")
set(CURL_INCLUDE_DIR "${CURL_ROOT}/include")
set(CURL_LIBRARY "${CURL_ROOT}/lib/libcurl.a")

# SDL2 Configuration
set(SDL2_ROOT "${CMAKE_SOURCE_DIR}/third_party/SDL2-2.28.5/x86_64-w64-mingw32")
if(EXISTS "${SDL2_ROOT}")
    set(SDL2_INCLUDE_DIR "${SDL2_ROOT}/include/SDL2")
    set(SDL2_LIBRARY "${SDL2_ROOT}/lib/libSDL2.a")
    set(SDL2_MAIN_LIBRARY "${SDL2_ROOT}/lib/libSDL2main.a")
    
    include_directories(${SDL2_INCLUDE_DIR})
    add_compile_definitions(ENABLE_SDL)
    message(STATUS "SDL2 found at ${SDL2_ROOT}")
else()
    message(WARNING "SDL2 not found at ${SDL2_ROOT}. Window support will be disabled.")
endif()

# QuickJS Configuration
set(QUICKJS_ROOT "${CMAKE_SOURCE_DIR}/third_party/quickjs")
if(EXISTS "${QUICKJS_ROOT}/quickjs.c")
    set(QUICKJS_SOURCES
        "${QUICKJS_ROOT}/quickjs.c"
        "${QUICKJS_ROOT}/libregexp.c"
        "${QUICKJS_ROOT}/libunicode.c"
        "${QUICKJS_ROOT}/cutils.c"
        "${QUICKJS_ROOT}/quickjs-libc.c"
        "${QUICKJS_ROOT}/dtoa.c"
    )
    
    # QuickJS is C, so we need to ensure C compiler is available but project handles C/CXX.
    # It often requires specific defines.
    add_library(quickjs STATIC ${QUICKJS_SOURCES})
    target_include_directories(quickjs PUBLIC "${QUICKJS_ROOT}")
    target_compile_definitions(quickjs PRIVATE 
        CONFIG_VERSION="2021-03-27"
        _GNU_SOURCE 
    )
    set_target_properties(quickjs PROPERTIES LINKER_LANGUAGE C)
    # Suppress warnings for third-party C code
    if(MSVC)
        target_compile_options(quickjs PRIVATE /w)
    else()
        target_compile_options(quickjs PRIVATE -w)
    endif()

    add_compile_definitions(ENABLE_QUICKJS)
    message(STATUS "QuickJS found at ${QUICKJS_ROOT}")
else()
    message(WARNING "QuickJS not found at ${QUICKJS_ROOT}. Scripting will be disabled.")
endif()


# FreeType and HarfBuzz support (bundled in third_party)
set(FREETYPE_ROOT "${CMAKE_SOURCE_DIR}/third_party/freetype")
set(HARFBUZZ_ROOT "${CMAKE_SOURCE_DIR}/third_party/harfbuzz")
option(ENABLE_FREETYPE "Enable FreeType and HarfBuzz support" ON)

if(ENABLE_FREETYPE AND EXISTS "${FREETYPE_ROOT}/lib/libfreetype.a" AND EXISTS "${HARFBUZZ_ROOT}/lib/libharfbuzz.a")
    add_compile_definitions(ENABLE_FREETYPE)
    include_directories(
        "${FREETYPE_ROOT}/include/freetype2"
        "${HARFBUZZ_ROOT}/include/harfbuzz"
    )
    set(FREETYPE_LIBRARIES
        "${HARFBUZZ_ROOT}/lib/libharfbuzz.a"
        "${FREETYPE_ROOT}/lib/libfreetype.a"
    )
    message(STATUS "FreeType and HarfBuzz enabled (bundled)")
elseif(ENABLE_FREETYPE)
    message(WARNING "ENABLE_FREETYPE is ON but libraries not found in third_party/. Disabling.")
    set(ENABLE_FREETYPE OFF)
endif()

# OpenGL Configuration (GPU accelerated rendering)
find_package(OpenGL)
if(OPENGL_FOUND)
    add_compile_definitions(ENABLE_OPENGL)
    message(STATUS "OpenGL found — GPU accelerated rendering enabled")
    # Add OpenGL include directories
    include_directories(${OPENGL_INCLUDE_DIR})
else()
    message(WARNING "OpenGL not found. GPU rendering will be disabled (Software fallback only).")
endif()

include_directories(include)
include_directories(${CURL_INCLUDE_DIR})

# Collect all source files
file(GLOB_RECURSE ALL_SOURCES "src/*.cpp")
# Exclude standalone demos and specific mains from the core source list
list(FILTER ALL_SOURCES EXCLUDE REGEX "simple_opengl_demo\\.cpp$")
list(FILTER ALL_SOURCES EXCLUDE REGEX "demo_minimal_main\\.cpp$")
list(FILTER ALL_SOURCES EXCLUDE REGEX "src/main\\.cpp$")

# Create an OBJECT library for the core framework to be reused by multiple targets
add_library(xiaopengkernel_core OBJECT ${ALL_SOURCES})
target_compile_definitions(xiaopengkernel_core PRIVATE 
    _CRT_SECURE_NO_WARNINGS
    CURL_STATICLIB
)
target_include_directories(xiaopengkernel_core PRIVATE ${CMAKE_SOURCE_DIR}/include ${CURL_INCLUDE_DIR})

if(MSVC)
    target_compile_options(xiaopengkernel_core PRIVATE 
        /W4
        /utf-8
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Od>
    )
else()
    target_compile_options(xiaopengkernel_core PRIVATE 
        -Wall
        -Wextra
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Debug>:-g>
    )
endif()

# The main executable
add_executable(${PROJECT_NAME} src/main.cpp $<TARGET_OBJECTS:xiaopengkernel_core>)

if(TARGET quickjs)
    target_include_directories(xiaopengkernel_core PRIVATE "${QUICKJS_ROOT}")
endif()

if(ENABLE_FREETYPE)
    target_include_directories(xiaopengkernel_core PRIVATE 
        "${FREETYPE_ROOT}/include/freetype2"
        "${HARFBUZZ_ROOT}/include/harfbuzz"
    )
endif()

if(EXISTS "${SDL2_ROOT}")
    target_include_directories(xiaopengkernel_core PRIVATE ${SDL2_INCLUDE_DIR})
endif()

if(OPENGL_FOUND)
    target_include_directories(xiaopengkernel_core PRIVATE ${OPENGL_INCLUDE_DIR})
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE 
    _CRT_SECURE_NO_WARNINGS
    CURL_STATICLIB
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /W4
        /utf-8
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Od>
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -Wall
        -Wextra
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Debug>:-g>
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${CURL_ROOT}/lib/libcurl.a
    ${CURL_ROOT}/lib/libnghttp2.a
    ${CURL_ROOT}/lib/libnghttp3.a
    ${CURL_ROOT}/lib/libngtcp2.a
    ${CURL_ROOT}/lib/libngtcp2_crypto_quictls.a
    ${CURL_ROOT}/lib/libssl.a
    ${CURL_ROOT}/lib/libcrypto.a
    ${CURL_ROOT}/lib/libssh2.a
    ${CURL_ROOT}/lib/libz.a
    ${CURL_ROOT}/lib/libzstd.a
    ${CURL_ROOT}/lib/libbrotlidec.a
    ${CURL_ROOT}/lib/libbrotlicommon.a
    ${CURL_ROOT}/lib/libpsl.a
    ws2_32
    crypt32
    wldap32
    secur32
    normaliz
    shlwapi
    bcrypt
)

if(EXISTS "${SDL2_ROOT}")
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        mingw32
        ${SDL2_MAIN_LIBRARY}
        ${SDL2_LIBRARY}
        winmm
        gdi32
        imm32
        ole32
        oleaut32
        version
        uuid
        advapi32
        setupapi
        shell32
        user32
    )
endif()

if(TARGET quickjs)
    target_link_libraries(${PROJECT_NAME} PRIVATE quickjs)
endif()

if(ENABLE_FREETYPE)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${FREETYPE_LIBRARIES} stdc++ rpcrt4)
endif()

if(OPENGL_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
    # Add OpenGL source files to core
    target_sources(xiaopengkernel_core PRIVATE 
        src/renderer/opengl_canvas.cpp
        src/renderer/opengl_context.cpp
    )
    
    # Add OpenGL test
    add_executable(test_opengl tests/test_opengl.cpp)
    target_include_directories(test_opengl PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_definitions(test_opengl PRIVATE SDL_MAIN_HANDLED)
    target_link_libraries(test_opengl PRIVATE 
        ${SDL2_MAIN_LIBRARY}
        ${SDL2_LIBRARY}
        opengl32
        winmm
        gdi32
        imm32
        ole32
        oleaut32
        version
        uuid
        advapi32
        setupapi
        shell32
        user32
    )
    set_target_properties(test_opengl PROPERTIES 
        WIN32_EXECUTABLE FALSE
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    )
    
    # Add simple OpenGL demo
    add_executable(simple_opengl_demo src/renderer/simple_opengl_demo.cpp)
    target_include_directories(simple_opengl_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_definitions(simple_opengl_demo PRIVATE SDL_MAIN_HANDLED)
    target_link_libraries(simple_opengl_demo PRIVATE 
        ${SDL2_MAIN_LIBRARY}
        ${SDL2_LIBRARY}
        opengl32
        winmm
        gdi32
        imm32
        ole32
        oleaut32
        version
        uuid
        advapi32
        setupapi
        shell32
        user32
    )
    set_target_properties(simple_opengl_demo PROPERTIES 
        WIN32_EXECUTABLE FALSE
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    )
    
    # Add GPU acceleration test
    add_executable(test_gpu_acceleration tests/test_gpu_acceleration.cpp)
    target_include_directories(test_gpu_acceleration PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_definitions(test_gpu_acceleration PRIVATE SDL_MAIN_HANDLED)
    target_link_libraries(test_gpu_acceleration PRIVATE 
        ${SDL2_MAIN_LIBRARY}
        ${SDL2_LIBRARY}
        opengl32
        winmm
        gdi32
        imm32
        ole32
        oleaut32
        version
        uuid
        advapi32
        setupapi
        shell32
        user32
    )
    set_target_properties(test_gpu_acceleration PROPERTIES 
        WIN32_EXECUTABLE FALSE
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

enable_testing()

# Test Layout
add_executable(test_layout tests/test_layout.cpp src/renderer/font_manager.cpp src/renderer/text_shaper.cpp)
target_include_directories(test_layout PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_layout PRIVATE 
    ${CURL_ROOT}/lib/libcurl.a
    ${CURL_ROOT}/lib/libnghttp2.a
    ${CURL_ROOT}/lib/libnghttp3.a
    ${CURL_ROOT}/lib/libngtcp2.a
    ${CURL_ROOT}/lib/libngtcp2_crypto_quictls.a
    ${CURL_ROOT}/lib/libssl.a
    ${CURL_ROOT}/lib/libcrypto.a
    ${CURL_ROOT}/lib/libssh2.a
    ${CURL_ROOT}/lib/libz.a
    ${CURL_ROOT}/lib/libzstd.a
    ${CURL_ROOT}/lib/libbrotlidec.a
    ${CURL_ROOT}/lib/libbrotlicommon.a
    ${CURL_ROOT}/lib/libpsl.a
    ws2_32 crypt32 wldap32 secur32 normaliz shlwapi iphlpapi bcrypt
)
target_compile_definitions(test_layout PRIVATE CURL_STATICLIB)
if(ENABLE_FREETYPE)
    target_link_libraries(test_layout PRIVATE ${FREETYPE_LIBRARIES} stdc++ rpcrt4)
endif()
set_target_properties(test_layout PROPERTIES WIN32_EXECUTABLE FALSE)

# Test Renderer
add_executable(test_renderer tests/test_renderer.cpp src/renderer/bitmap_canvas.cpp src/renderer/painting_algorithm.cpp src/renderer/image.cpp src/renderer/font_manager.cpp src/renderer/text_shaper.cpp)
target_include_directories(test_renderer PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_renderer PRIVATE 
    ${CURL_ROOT}/lib/libcurl.a
    ${CURL_ROOT}/lib/libnghttp2.a
    ${CURL_ROOT}/lib/libnghttp3.a
    ${CURL_ROOT}/lib/libngtcp2.a
    ${CURL_ROOT}/lib/libngtcp2_crypto_quictls.a
    ${CURL_ROOT}/lib/libssl.a
    ${CURL_ROOT}/lib/libcrypto.a
    ${CURL_ROOT}/lib/libssh2.a
    ${CURL_ROOT}/lib/libz.a
    ${CURL_ROOT}/lib/libzstd.a
    ${CURL_ROOT}/lib/libbrotlidec.a
    ${CURL_ROOT}/lib/libbrotlicommon.a
    ${CURL_ROOT}/lib/libpsl.a
    ws2_32 crypt32 wldap32 secur32 normaliz shlwapi iphlpapi bcrypt
)
target_compile_definitions(test_renderer PRIVATE CURL_STATICLIB)
if(ENABLE_FREETYPE)
    target_link_libraries(test_renderer PRIVATE ${FREETYPE_LIBRARIES} stdc++ rpcrt4)
endif()
set_target_properties(test_renderer PROPERTIES WIN32_EXECUTABLE FALSE)

file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
list(FILTER TEST_SOURCES EXCLUDE REGEX "test_layout.cpp")
list(FILTER TEST_SOURCES EXCLUDE REGEX "test_renderer.cpp")
list(FILTER TEST_SOURCES EXCLUDE REGEX "test_parse_thread.cpp")
list(FILTER TEST_SOURCES EXCLUDE REGEX "test_opengl.cpp")
list(FILTER TEST_SOURCES EXCLUDE REGEX "test_gpu_acceleration.cpp")
list(FILTER TEST_SOURCES EXCLUDE REGEX "test_resource_tree.cpp")

# Exclude flexbox tests from loader_tests to avoid multiple mains
list(FILTER TEST_SOURCES EXCLUDE REGEX "test_flexbox.cpp")
list(FILTER TEST_SOURCES EXCLUDE REGEX "test_flexbox_advanced.cpp")

add_executable(loader_tests ${TEST_SOURCES})

target_include_directories(loader_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CURL_INCLUDE_DIR}
)

target_compile_definitions(loader_tests PRIVATE 
    _CRT_SECURE_NO_WARNINGS
    CURL_STATICLIB
)

if(MSVC)
    target_compile_options(loader_tests PRIVATE 
        /W4
        /utf-8
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Od>
    )
else()
    target_compile_options(loader_tests PRIVATE 
        -Wall
        -Wextra
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Debug>:-g>
    )
endif()

target_link_libraries(loader_tests PRIVATE 
    ${CURL_ROOT}/lib/libcurl.a
    ${CURL_ROOT}/lib/libnghttp2.a
    ${CURL_ROOT}/lib/libnghttp3.a
    ${CURL_ROOT}/lib/libngtcp2.a
    ${CURL_ROOT}/lib/libngtcp2_crypto_quictls.a
    ${CURL_ROOT}/lib/libssl.a
    ${CURL_ROOT}/lib/libcrypto.a
    ${CURL_ROOT}/lib/libssh2.a
    ${CURL_ROOT}/lib/libz.a
    ${CURL_ROOT}/lib/libzstd.a
    ${CURL_ROOT}/lib/libbrotlidec.a
    ${CURL_ROOT}/lib/libbrotlicommon.a
    ${CURL_ROOT}/lib/libpsl.a
    ws2_32
    crypt32
    wldap32
    secur32
    normaliz
    shlwapi
    iphlpapi
    bcrypt
)

# Add SDL2 and OpenGL libraries if available
if(EXISTS "${SDL2_ROOT}")
    target_link_libraries(loader_tests PRIVATE 
        mingw32
        ${SDL2_MAIN_LIBRARY}
        ${SDL2_LIBRARY}
        winmm
        gdi32
        imm32
        ole32
        oleaut32
        version
        uuid
        advapi32
        setupapi
        shell32
        user32
    )
    target_compile_definitions(loader_tests PRIVATE SDL_MAIN_HANDLED)
endif()

if(OPENGL_FOUND)
    target_link_libraries(loader_tests PRIVATE opengl32)
endif()

set_target_properties(loader_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

add_test(NAME LoaderTests COMMAND loader_tests)

# Test ParseThread (multi-threaded parsing)
add_executable(test_parse_thread tests/test_parse_thread.cpp)
target_include_directories(test_parse_thread PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_parse_thread PRIVATE 
    ${CURL_ROOT}/lib/libcurl.a
    ${CURL_ROOT}/lib/libnghttp2.a
    ${CURL_ROOT}/lib/libnghttp3.a
    ${CURL_ROOT}/lib/libngtcp2.a
    ${CURL_ROOT}/lib/libngtcp2_crypto_quictls.a
    ${CURL_ROOT}/lib/libssl.a
    ${CURL_ROOT}/lib/libcrypto.a
    ${CURL_ROOT}/lib/libssh2.a
    ${CURL_ROOT}/lib/libz.a
    ${CURL_ROOT}/lib/libzstd.a
    ${CURL_ROOT}/lib/libbrotlidec.a
    ${CURL_ROOT}/lib/libbrotlicommon.a
    ${CURL_ROOT}/lib/libpsl.a
    ws2_32 crypt32 wldap32 secur32 normaliz shlwapi iphlpapi bcrypt
)
target_compile_definitions(test_parse_thread PRIVATE CURL_STATICLIB)
set_target_properties(test_parse_thread PROPERTIES 
    WIN32_EXECUTABLE FALSE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
add_test(NAME ParseThreadTests COMMAND test_parse_thread)

# Test ResourceTree (resource dependency management)
add_executable(test_resource_tree tests/test_resource_tree.cpp)
target_include_directories(test_resource_tree PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_resource_tree PRIVATE 
    ${CURL_ROOT}/lib/libcurl.a
    ${CURL_ROOT}/lib/libnghttp2.a
    ${CURL_ROOT}/lib/libnghttp3.a
    ${CURL_ROOT}/lib/libngtcp2.a
    ${CURL_ROOT}/lib/libngtcp2_crypto_quictls.a
    ${CURL_ROOT}/lib/libssl.a
    ${CURL_ROOT}/lib/libcrypto.a
    ${CURL_ROOT}/lib/libssh2.a
    ${CURL_ROOT}/lib/libz.a
    ${CURL_ROOT}/lib/libzstd.a
    ${CURL_ROOT}/lib/libbrotlidec.a
    ${CURL_ROOT}/lib/libbrotlicommon.a
    ${CURL_ROOT}/lib/libpsl.a
    ws2_32 crypt32 wldap32 secur32 normaliz shlwapi iphlpapi bcrypt
)
target_compile_definitions(test_resource_tree PRIVATE CURL_STATICLIB)
set_target_properties(test_resource_tree PROPERTIES 
    WIN32_EXECUTABLE FALSE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
add_test(NAME ResourceTreeTests COMMAND test_resource_tree)

# Test Flexbox
add_executable(test_flexbox tests/test_flexbox.cpp)
target_include_directories(test_flexbox PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_flexbox PRIVATE 
    ${CURL_ROOT}/lib/libcurl.a
    ${CURL_ROOT}/lib/libnghttp2.a
    ${CURL_ROOT}/lib/libnghttp3.a
    ${CURL_ROOT}/lib/libngtcp2.a
    ${CURL_ROOT}/lib/libngtcp2_crypto_quictls.a
    ${CURL_ROOT}/lib/libssl.a
    ${CURL_ROOT}/lib/libcrypto.a
    ${CURL_ROOT}/lib/libssh2.a
    ${CURL_ROOT}/lib/libz.a
    ${CURL_ROOT}/lib/libzstd.a
    ${CURL_ROOT}/lib/libbrotlidec.a
    ${CURL_ROOT}/lib/libbrotlicommon.a
    ${CURL_ROOT}/lib/libpsl.a
    ws2_32 crypt32 wldap32 secur32 normaliz shlwapi iphlpapi bcrypt
)
target_compile_definitions(test_flexbox PRIVATE CURL_STATICLIB)
set_target_properties(test_flexbox PROPERTIES 
    WIN32_EXECUTABLE FALSE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
add_test(NAME FlexboxTests COMMAND test_flexbox)

# Test Flexbox Advanced
add_executable(test_flexbox_advanced tests/test_flexbox_advanced.cpp)
target_include_directories(test_flexbox_advanced PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_flexbox_advanced PRIVATE 
    ${CURL_ROOT}/lib/libcurl.a
    ${CURL_ROOT}/lib/libnghttp2.a
    ${CURL_ROOT}/lib/libnghttp3.a
    ${CURL_ROOT}/lib/libngtcp2.a
    ${CURL_ROOT}/lib/libngtcp2_crypto_quictls.a
    ${CURL_ROOT}/lib/libssl.a
    ${CURL_ROOT}/lib/libcrypto.a
    ${CURL_ROOT}/lib/libssh2.a
    ${CURL_ROOT}/lib/libz.a
    ${CURL_ROOT}/lib/libzstd.a
    ${CURL_ROOT}/lib/libbrotlidec.a
    ${CURL_ROOT}/lib/libbrotlicommon.a
    ${CURL_ROOT}/lib/libpsl.a
    ws2_32 crypt32 wldap32 secur32 normaliz shlwapi iphlpapi bcrypt
)
target_compile_definitions(test_flexbox_advanced PRIVATE CURL_STATICLIB)
set_target_properties(test_flexbox_advanced PROPERTIES 
    WIN32_EXECUTABLE FALSE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
add_test(NAME FlexboxAdvancedTests COMMAND test_flexbox_advanced)

add_custom_target(run
    COMMAND ${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

## Minimal Demo: standalone executable for end-to-end demonstration
add_executable(minimal_demo src/demo_minimal_main.cpp $<TARGET_OBJECTS:xiaopengkernel_core>)
target_include_directories(minimal_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(minimal_demo PRIVATE
  ${CURL_ROOT}/lib/libcurl.a
  ${CURL_ROOT}/lib/libnghttp2.a
  ${CURL_ROOT}/lib/libnghttp3.a
  ${CURL_ROOT}/lib/libngtcp2.a
  ${CURL_ROOT}/lib/libngtcp2_crypto_quictls.a
  ${CURL_ROOT}/lib/libssl.a
  ${CURL_ROOT}/lib/libcrypto.a
  ${CURL_ROOT}/lib/libssh2.a
  ${CURL_ROOT}/lib/libz.a
  ${CURL_ROOT}/lib/libzstd.a
  ${CURL_ROOT}/lib/libbrotlidec.a
  ${CURL_ROOT}/lib/libbrotlicommon.a
  ${CURL_ROOT}/lib/libpsl.a
  ws2_32 crypt32 wldap32 secur32 normaliz shlwapi iphlpapi bcrypt
)
if(EXISTS "${SDL2_ROOT}")
  target_link_libraries(minimal_demo PRIVATE mingw32
      ${SDL2_MAIN_LIBRARY}
      ${SDL2_LIBRARY}
      winmm
      gdi32
      imm32
      ole32
      oleaut32
      version
      uuid
      advapi32
      setupapi
      shell32
      user32
  )
endif()
if(OPENGL_FOUND)
  target_link_libraries(minimal_demo PRIVATE opengl32)
endif()
if(TARGET quickjs)
  target_link_libraries(minimal_demo PRIVATE quickjs)
endif()
if(ENABLE_FREETYPE)
  target_link_libraries(minimal_demo PRIVATE ${FREETYPE_LIBRARIES} stdc++ rpcrt4)
endif()
set_target_properties(minimal_demo PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
add_custom_target(run_tests
    COMMAND loader_tests
    DEPENDS loader_tests
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake_clean.cmake
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_SOURCE_DIR}/bin"
)
