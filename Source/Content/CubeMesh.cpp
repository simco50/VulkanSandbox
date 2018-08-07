#include "stdafx.h"
#include "CubeMesh.h"
#include "Resource/IndexBuffer.h"
#include "Resource/VertexBuffer.h"

CubeMesh::CubeMesh(Graphics* pGraphics) :
	Mesh(pGraphics)
{
	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoord;
	};

	float hw = 1.0f / 2.0f;
	float hh = 1.0f / 2.0f;
	float hd = 1.0f / 2.0f;

	//Vertex buffer
	std::vector<Vertex> vertices =
	{
		//front 0
		{ glm::vec3(-hw, hh, -hd), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0, 0) },
	{ glm::vec3(hw, hh, -hd), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1, 0) },
	{ glm::vec3(hw, -hh, -hd), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1, 1) },
	{ glm::vec3(-hw, -hh, -hd), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1, 0) },

	//top 4
	{ glm::vec3(-hw, hh, -hd), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0, 0) },
	{ glm::vec3(-hw, hh, hd), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1, 0) },
	{ glm::vec3(hw, hh, hd), glm::vec3(0.0f, 1.0f, 0.0f) , glm::vec2(1, 1) },
	{ glm::vec3(hw, hh, -hd), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1, 0) },

	//left 8
	{ glm::vec3(-hw, hh, -hd), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0, 0) },
	{ glm::vec3(-hw, hh, hd), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1, 0) },
	{ glm::vec3(-hw, -hh, hd), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1, 1) },
	{ glm::vec3(-hw, -hh, -hd), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1, 0) },

	//right 12
	{ glm::vec3(hw, hh, -hd), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0, 0) },
	{ glm::vec3(hw, hh, hd), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1, 0) },
	{ glm::vec3(hw, -hh, hd), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1, 1) },
	{ glm::vec3(hw, -hh, -hd), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1, 0) },

	//bottom 16
	{ glm::vec3(-hw, -hh, -hd), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0, 0) },
	{ glm::vec3(-hw, -hh, hd), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1, 0) },
	{ glm::vec3(hw, -hh, hd), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1, 1) },
	{ glm::vec3(hw, -hh, -hd), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1, 0) },

	//back 20
	{ glm::vec3(-hw, hh, hd), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, 0) },
	{ glm::vec3(hw, hh, hd), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1, 0) },
	{ glm::vec3(hw, -hh, hd) , glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1, 1) },
	{ glm::vec3(-hw, -hh, hd), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1, 0) },
	};

	std::vector<unsigned int> indices = {
		0, 1, 2,
		0, 2, 3,

		4, 5, 6,
		4, 6, 7,

		9, 8, 10,
		10, 8, 11,

		12, 13, 14,
		14, 15, 12,

		16, 18, 17,
		19, 18, 16,

		20, 23, 22,
		21, 20, 22
	};

	m_pIndexBuffer = std::make_unique<IndexBuffer>(pGraphics);
	m_pIndexBuffer->SetSize((int)indices.size(), false, false);
	m_pIndexBuffer->SetData(indices.data());

	m_pVertexBuffer = std::make_unique<VertexBuffer>(pGraphics);
	m_pVertexBuffer->SetSize((int)vertices.size() * sizeof(Vertex));
	m_pVertexBuffer->SetData((int)vertices.size(), 0, vertices.data());
}

CubeMesh::~CubeMesh()
{
}