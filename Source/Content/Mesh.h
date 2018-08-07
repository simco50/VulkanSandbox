#pragma once

class IndexBuffer;
class VertexBuffer;
class Graphics;

class Mesh
{
public:
	Mesh(Graphics* pGraphics);
	virtual ~Mesh();

	virtual bool Load(const std::string& path) { return false; }

	IndexBuffer* GetIndexBuffer() const { return m_pIndexBuffer.get(); }
	VertexBuffer* GetVertexBuffer() const { return m_pVertexBuffer.get(); }

protected:
	std::unique_ptr<IndexBuffer> m_pIndexBuffer;
	std::unique_ptr<VertexBuffer> m_pVertexBuffer;

	Graphics* m_pGraphics;
};