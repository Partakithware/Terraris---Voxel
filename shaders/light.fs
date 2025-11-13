// shaders/light.fs
#version 460 core
out vec4 FragColor;
uniform vec3 cubeColor; // <--- The uniform must exist

void main() {
    // Light source should be pure white and ignore all lighting calculations
    //FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f); 
    FragColor = vec4(cubeColor, 1.0); // <--- It must be used
}
