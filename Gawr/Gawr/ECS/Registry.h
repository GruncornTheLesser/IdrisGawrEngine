#pragma once
#include "HandleManager.h"
#include "Pool.h"
#include "Dispatcher.h"


namespace Gawr::ECS 
{
	template<typename ... comp_ts>
	class Registry : public HandleManager
	{
	public:
		template<typename ... ts>
		class View;

		Registry() = default;
		Registry(const Registry&) = delete;
		const Registry& operator=(const Registry&) = delete;

		template<typename ... filter_ts>
		View<filter_ts...> view() {
			return View<filter_ts...>(*this);
		}

		template<typename t> requires (std::is_same_v<t, comp_ts> || ...)
		Pool<t>& pool() {
			return std::get<Pool<t>>(m_storage);
		}

	private:
		std::tuple<Pool<comp_ts>...> m_storage;
	};
}
