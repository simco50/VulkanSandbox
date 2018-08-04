#pragma once
class IndexBuffer;
class VertexBuffer;
class UniformBuffer;
class Shader;
class Graphics;
class Material;

class Drawable
{
public:
	Drawable(Graphics* pGraphics);
	~Drawable();

	IndexBuffer* GetIndexBuffer() const { return m_pIndexBuffer.get(); }
	VertexBuffer* GetVertexBuffer() const { return m_pVertexBuffer.get(); }

	glm::mat4 GetWorldMatrix() const;
	void SetPosition(float x, float y, float z);
	void SetRotation(float x, float y, float z, float angle);
	void SetScale(float x, float y, float z);

	void SetMaterial(Material* pMaterial) { m_pMaterial = pMaterial; }
	Material* GetMaterial() const { return m_pMaterial; }

protected:
	Material * m_pMaterial = nullptr;
	Graphics* m_pGraphics;

	std::unique_ptr<IndexBuffer> m_pIndexBuffer;
	std::unique_ptr<VertexBuffer> m_pVertexBuffer;

	glm::vec3 m_Position = {};
	glm::quat m_Rotation = {};
	glm::vec3 m_Scale = {1,1,1};
};
