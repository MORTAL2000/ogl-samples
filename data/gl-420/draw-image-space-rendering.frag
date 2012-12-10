#version 420 core

#include "fbo-image-space-rendering.glsl"

layout(binding = DIFFUSE) uniform sampler2D Diffuse;

const vec2 FRAMEBUFFER_STEP = 1.0 / vec2(640, 480);

in vec4 gl_FragCoord;
layout(location = FRAG_COLOR, index = 0) out vec4 Color;

void main()
{
	Color = texture(Diffuse, vec2(gl_FragCoord.x, 1.0 - gl_FragCoord.y) * FRAMEBUFFER_STEP);
}
