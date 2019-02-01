#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    mat4 correction = 
        mat4(
            vec4(1, 0, 0, 0),
            vec4(0, -1, 0, 0), 
            vec4(0, 0, 0.5, 0.0),
            vec4(0, 0, 0.5, 1)
            );

    gl_Position = correction * vec4(inPosition, 1.0);
    fragColor = inColor;
}