#pragma once

#include "Drawable.h"

class Graphics;

class Cube : public Drawable
{
public:
	Cube(Graphics* pGraphics);
	~Cube();

	void Create(float width, float height, float depth);
};

