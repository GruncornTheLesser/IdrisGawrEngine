#pragma once
#include "Core/Window.h"


namespace Gawr {
	class RendererBase {
	public:
		void refresh() const;

	private:
	};

	class Application : Gawr::Core::Window
	{
	public:
		//Scene<RendererTest> scene;

		void run() {
			while (!closed())
			{
				//scene.draw();
			}
		}

		Application() : Gawr::Core::Window(800, 600, "Gawr") { 
			run();
		}
	};
}

