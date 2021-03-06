#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, binding = 2, set = 0) uniform PerFrameData 
{
	float deltaTime;
	int frameCount;
} perFrameData;

layout (std140, binding = 0, set = 2) uniform PerModelData 
{
	mat4 m;
    mat4 mvp;
} perModelData;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outTexCoord;

void main() 
{
	outNormal = mat3(perModelData.m) * inNormal;
	outTexCoord = inTexCoord;

	gl_Position = perModelData.mvp * vec4(pos, 1);
}