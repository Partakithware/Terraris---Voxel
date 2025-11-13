// shaders/ray.fs
#version 460 core
out vec4 FragColor;

uniform vec3 rayColor; // Uniform to set the ray color

void main() {
    FragColor = vec4(rayColor, 1.0);
}
