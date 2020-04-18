#version 330

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in float v_zValue;
layout(location = 3) in vec2 v_imageSize;
layout(location = 4) in vec2 v_imageOffset;
layout(location = 5) in vec4 v_hoverColor;
layout(location = 6) in vec2 v_atlasOffset;

layout(std140) uniform vertexUniforms
{
    vec2 screenSize;

    vec2 pad2;
};

out vec2 uv;
flat out vec2 imageSize;
flat out vec4 hoverColor;
flat out vec2 atlasOffset;
flat out float f_zValue;

void main()
{
    uv = v_uv;
    imageSize = v_imageSize;
    hoverColor = v_hoverColor;
    atlasOffset = v_atlasOffset;
    f_zValue = v_zValue;
    
    gl_Position = vec4((vertex_position.xy * v_imageSize + v_imageOffset) / screenSize * 2.0, 0.0, 1.0);
    gl_Position.x = gl_Position.x - 1.0;
    gl_Position.y = 1.0 - gl_Position.y;
}
