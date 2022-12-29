#pragma once
#include "Entity.h"
#include "Registry.h"

namespace Gawr::ECS {
	/*
	template<typename filter_t>
	concept isFilter = std::is_same_v<decltype(filter_t::isValid(std::declval<Registry<>&>(), std::declval<entity_t>())), bool>;

	template<typename filter_t>
	concept isGetPolicy = std::is_same_v<decltype(filter_t::get(std::declval<Registry<>&>(), std::declval<entity_t>())), bool>;
	*/
	
	template<typename ... get_ts>
	struct custom_get {
		// get policy
		using orderby = std::tuple_element_t<0, std::tuple<get_ts...>>;

		template<typename ... comp_ts>
		static inline auto get(Registry<comp_ts...>& reg, entity_t e) {
			return std::tuple_cat(std::tuple(e), get_packed<Registry<comp_ts...>, get_ts>(reg, e)...);
		}

	private:
		template<typename registry_t, typename t>
		static inline auto get_packed(registry_t& reg, entity_t e) {
			if constexpr (std::is_empty_v<t>)	return std::tuple<>{};
			else								return std::tuple<t&>(reg.pool<t>().get(e));
		}
	};

	template<typename ... include_ts>
	struct include : public custom_get<include_ts...> {
		// filter
		template<typename ... comp_ts>
		static inline bool isValid(Registry<comp_ts...>& reg, entity_t e) {
			return (reg.pool<include_ts>().has(e) && ...);
		}
	};

	template<typename ... exclude_ts>
	struct exclude {
		// filter
		template<typename ... comp_ts>
		static inline bool isValid(Registry<comp_ts...>& reg, entity_t e) {
			return (!reg.pool<exclude_ts>().has(e) && ...);
		}
	};


}