#pragma once
#include "HandleManager.h"
#include "Pool.h"
#include <functional>
namespace Gawr::ECS {
	template<typename ... args_t>
	class EventDispatcher
	{
		using handle_t = uint32_t;
		using event_t = std::function<void(const args_t...)>;

	public:
		handle_t connect(event_t event) {
			uint32_t handle = m_handleManager.create();
			m_storage.emplace(handle, event);
			return handle;
		}
		void disconnect(uint32_t handle) {
			m_storage.erase(handle);
			m_handleManager.destroy();
		}

		void dispatch(const args_t& ... args) 
		{		
			std::for_each(m_storage.begin(), m_storage.end(), [&](auto id) { 
				m_storage.get_i(id)(args...);
				});
		}
	private:
		Pool<handle_t, event_t> m_storage;
		HandleManager m_handleManager;

	};
}