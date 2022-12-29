#pragma once
#include "Core/Window.h"

namespace Gawr {
	
	class Application
	{
	public:
		void run() {
			while (!m_window.closed())
			{
				//scene.draw();
			}
		}

		Application() : m_window("app", 800, 600, false) {
			run();
		}
	private:
		Gawr::Core::Window m_window;
	};
}

