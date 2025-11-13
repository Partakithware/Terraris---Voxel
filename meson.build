# meson.build

project('Terraris', ['cpp', 'c'],
    version : '0.1.0',
    default_options : ['cpp_std=c++17']
)

# --- 1. Dependencies ---

# GLFW (Finds the library installed via apt: libglfw3-dev)
glfw = dependency('glfw3')

# OpenGL
opengl = dependency('gl')

# GLM (Finds the library installed via apt: libglm-dev)
glm = dependency('glm')

# --- 2. Source Files ---

# All source files, noting that main.cpp is now in src/
# and glad.c is also in src/
sources = [
    'src/main.cpp',
    'src/glad.c',
    'src/Camera.cpp',
    'src/Shader.cpp'
]

# --- 3. Executable and Linkage ---

executable('Terraris', 
    sources,
    # This points to the parent directory of Camera.h and the glad/ folder.
    include_directories : ['include'], 
    dependencies : [glfw, opengl, glm],
    install : true
)