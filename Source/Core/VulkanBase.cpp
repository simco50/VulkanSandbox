#include "stdafx.h"
#include "Graphics.h"

int main()
{
	Graphics* pGraphics = new Graphics();
	pGraphics->Initialize();
	pGraphics->Shutdown();
	delete pGraphics;
    return 0;
}