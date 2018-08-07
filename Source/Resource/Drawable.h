#pragma once
class IndexBuffer;
class VertexBuffer;
class UniformBuffer;
class Shader;
class Graphics;
class Material;
class Mesh;

class Drawable
{
public:
	Drawable(Graphics* pGraphics, Mesh* pMesh);
	~Drawable();

	Mesh* GetMesh() const { return m_pMesh; }

	glm::mat4 GetWorldMatrix() const;
	void SetPosition(float x, float y, float z);
	void SetRotation(float x, float y, float z, float angle);
	void SetScale(float x, float y, float z);

	void SetMaterial(Material* pMaterial) { m_pMaterial = pMaterial; }
	Material* GetMaterial() const { return m_pMaterial; }

protected:
	Material * m_pMaterial = nullptr;
	Graphics* m_pGraphics;
	Mesh* m_pMesh;

	glm::vec3 m_Position = {};
	glm::quat m_Rotation = {};
	glm::vec3 m_Scale = {1,1,1};
};
