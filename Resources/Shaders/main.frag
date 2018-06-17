#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 texCoord;
layout (location = 0) out vec4 outColor;

void main() 
{
	vec3 lightPosition = vec3(0.6f, 1.0f, -1.0f);
	vec3 lighDirection = normalize(lightPosition);

	vec4 c = texture(samplerColor, texCoord, 0);

	vec3 n = normalize(normal);
	float diffuse = dot(n, lighDirection);

	outColor = diffuse * c;
}