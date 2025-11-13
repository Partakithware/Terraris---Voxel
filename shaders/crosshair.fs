// shaders/crosshair.fs
#version 460 core
out vec4 FragColor;

uniform vec3 crosshairColor; // Can be used to change color

void main() {
    FragColor = vec4(crosshairColor, 1.0);
}
