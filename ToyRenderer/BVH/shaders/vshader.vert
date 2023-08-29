#version 330 core

layout (location = 0) in vec3 vPosition;  // cpu传入的顶点坐标

out vec3 pix;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

void main() {
    vec4 position = projection * view * model * vec4(vPosition, 1.0);
    gl_Position = position;
}

