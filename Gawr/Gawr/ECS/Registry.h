#pragma once
#include "HandleManager.h"
#include "EventDispatcher.h"
#include "View.h"

namespace Gawr::ECS 
{
	template<typename handle_t, typename ... comp_ts>
	class Registry : public HandleManager, private Pool<handle_t, comp_ts>...
	{
	public:
		Registry() = default;
		Registry(const Registry&) = delete;
		const Registry& operator=(const Registry&) = delete;

		template<typename ... view_ts>
		View<Registry, view_ts...>& view() {
			return *reinterpret_cast<View<Registry, view_ts...>*>(this);
		}

		// storage methods
		template<typename comp_t>
		Pool<handle_t, comp_t>& pool() {
			return *static_cast<Pool<handle_t, comp_t>*>(this);
		}

		template<typename ... has_ts>
		bool has(uint32_t e) const {
			return (Pool<handle_t, has_ts>::has(e) && ...);
		}

		template<typename comp_t>
		comp_t& get_i(uint32_t e) {
			return Pool<handle_t, comp_t>::get_i(Pool<handle_t, comp_t>::index(e));
		}

		template<typename comp_t>
		comp_t* try_get(uint32_t e) {
			return Pool<handle_t, comp_t>::has(e) ? &Pool<handle_t, comp_t>::get_i(e) : nullptr;
		}

		template<typename comp_t, typename ... args_ts>
		auto emplace(uint32_t e, args_ts&& ... args) {
			return Pool<handle_t, comp_t>::emplace(e, args...);
		}

		template<typename comp_t>
		void erase(uint32_t e) {
			Pool<handle_t, comp_t>::erase(e);
		}
		
	};
}