#include "stdafx.h"
#include "Drawable.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"

Drawable::Drawable(Graphics* pGraphics, Mesh* pMesh):
	m_pGraphics(pGraphics), m_pMesh(pMesh)
{

}

Drawable::~Drawable()
{
}

glm::mat4 Drawable::GetWorldMatrix() const
{
	return glm::translate(glm::mat4(1),m_Position) * glm::toMat4(m_Rotation) * glm::scale(glm::mat4(1), m_Scale);
}

void Drawable::SetPosition(float x, float y, float z)
{
	m_Position = glm::vec3(x, y, z);
}

void Drawable::SetRotation(float x, float y, float z, float angle)
{
	m_Rotation = glm::angleAxis(angle, glm::vec3(x, y, z));
}

void Drawable::SetScale(float x, float y, float z)
{
	m_Scale = glm::vec3(x, y, z); 
}
