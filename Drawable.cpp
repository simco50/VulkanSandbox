#include "stdafx.h"
#include "Drawable.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"

Drawable::Drawable(Graphics* pGraphics) : 
	m_pGraphics(pGraphics)
{
}


Drawable::~Drawable()
{
}

glm::mat4 Drawable::GetWorldMatrix() const
{
	return glm::translate(glm::mat4(1), m_Position) * m_Rotation;
}

void Drawable::SetRotation(float x, float y, float z, float angle)
{
	m_Rotation = glm::rotate(glm::mat4(1), angle, glm::vec3(x, y, z));
}
