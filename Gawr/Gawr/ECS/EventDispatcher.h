#pragma once
#include "Entity.h"
#include "HandleManager.h"
#include "Pool.h"
#include <functional>

namespace Gawr::ECS {
	template<typename ... args_t>
	class EventDispatcher
	{
		using event_t = std::function<void(const args_t...)>;

	public:
		entity_t connect(event_t event) {
			entity_t handle = m_handleManager.create();
			m_storage.emplace(handle, event);
			return handle;
		}
		void disconnect(entity_t handle) {
			m_storage.erase(handle);
			m_handleManager.destroy();
		}
		void dispatch(const args_t& ... args) 
		{		
			std::for_each(m_storage.begin(), m_storage.end(), [&](auto id) { 
				m_storage.get_at(id)(args...);
				});
		}
	
	private:
		Pool<event_t> m_storage;
		HandleManager m_handleManager;
	};
}