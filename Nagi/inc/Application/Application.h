#pragma once

namespace Nagi
{

class Window;
class GraphicsContext;

class Application
{
public:
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
protected:
	Application() = delete;
	~Application() = default;
	Application(Window& window, GraphicsContext& gfxCon);

	// Default application behaviour that exists on all Applications should be here
	
protected:
	Window& m_window;
	GraphicsContext& m_gfxCon;

};
}



