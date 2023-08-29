#version 330 core

out vec4 FragColor;    // 片元输出像素的颜色

uniform vec3 color;

void main() {
    FragColor = vec4(color, 1.0);
}
