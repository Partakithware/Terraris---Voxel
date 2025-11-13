// shaders/crosshair.vs
#version 460 core
layout (location = 0) in vec2 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float pointSize; // NEW UNIFORM FOR SIZE

void main() {
    gl_Position = projection * view * model * vec4(aPos, 0.0, 1.0);
    gl_PointSize = pointSize; // CRITICAL: Set the size here
}