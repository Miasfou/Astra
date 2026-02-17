cmake_minimum_required(VERSION 3.10)
project(AstraChess)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. FORCE STATIC LINKING (No DLL dependencies) ---
# Bake the C++ Runtime (MSVC) into the EXE
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Force GLFW to be built as a static library
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# --- 2. ADD SOURCES ---
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# --- 3. EXTERNAL LIBS ---
# Added third_party/stb to the include path so stb_image.h is found
add_subdirectory(third_party/glfw)

# --- 4. EXECUTABLE ---
add_executable(AstraChess ${SOURCES})

# --- 5. LINKING & INCLUDES ---
target_include_directories(AstraChess PRIVATE 
    src 
    third_party/glfw/include 
    third_party/stb
)

target_link_libraries(AstraChess PRIVATE glfw)

# Windows specific static system libs for GLFW
if(WIN32)
    target_link_libraries(AstraChess PRIVATE opengl32 gdi32 user32 shell32)
    
    # Hide the console window in the background for a "clean" app look
    # Remove the next line if you still want to see the Astra> console
    # set_target_properties(AstraChess PROPERTIES WIN32_EXECUTABLE TRUE)
endif()