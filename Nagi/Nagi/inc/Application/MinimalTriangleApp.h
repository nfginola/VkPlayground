#pragma once
#include "Application.h"

namespace Nagi
{

/*

Creates a triangle with minimal resources!
We are drawing it with the immediate buffer in the shader which
we index with the help of simple vertexID and instanceID

*/

class MinimalTriangleApp : public Application
{
public:
	MinimalTriangleApp(Window& window, GraphicsContext& gfxCon);
	~MinimalTriangleApp() = default;

	MinimalTriangleApp() = delete;
	MinimalTriangleApp& operator=(const Application&) = delete;

private:
	int a;

};


}
