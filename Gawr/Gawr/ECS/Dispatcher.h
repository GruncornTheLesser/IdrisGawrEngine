#pragma once
#include "Entity.h"

namespace Gawr::ECS {
	template<typename ... arg_ts>
	class Dispatcher {
	public:
		using event_t = void(*)(arg_ts...);

		void connect(event_t func) noexcept {
			auto it = std::find(m_storage.begin(), m_storage.end(), func);
			
			if (it == m_storage.end()) 
				m_storage.push_back(func);
		}

		void disconnect(event_t func) noexcept {
			auto it = std::find(m_storage.begin(), m_storage.end(), func);

			if (it != m_storage.end())
				m_storage.erase(it);
		}
		void dispatch(arg_ts ... args) noexcept
		{
			for (event_t func : m_storage)
				func(args...);
		}

	private:
		std::vector<event_t> m_storage;
	};
	

}