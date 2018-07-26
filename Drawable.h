#pragma once
class IndexBuffer;
class VertexBuffer;
class UniformBuffer;
class Shader;
class Graphics;

class Drawable
{
public:
	Drawable(Graphics* pGraphics);
	~Drawable();

	IndexBuffer* GetIndexBuffer() const { return m_pIndexBuffer.get(); }
	VertexBuffer* GetVertexBuffer() const { return m_pVertexBuffer.get(); }

	glm::mat4 GetWorldMatrix() const;
	void SetPosition(float x, float y, float z) { m_Position = glm::vec3(x, y, z); }
	void SetRotation(float x, float y, float z, float angle);

protected:
	Graphics* m_pGraphics;

	std::unique_ptr<IndexBuffer> m_pIndexBuffer;
	std::unique_ptr<VertexBuffer> m_pVertexBuffer;

	glm::vec3 m_Position;
	glm::mat4 m_Rotation = glm::mat4(1);
};
