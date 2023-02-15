#pragma once
#include "Pipeline.h"
#include "Pool.h"
#include "View.h"

#include <tuple>

namespace Gawr::ECS {
	template<typename ... Ts>
	class Registry {
		using pool_collection_t = std::tuple<Pool<std::remove_const_t<Ts>>...>;

		template<typename U>
		using pool_t = Pool<std::remove_const_t<U>>;

		template<typename U>
		using pool_reference_t = std::conditional_t<std::is_const_v<U>, const Pool<std::remove_const_t<U>>&, Pool<std::remove_const_t<U>>&>;

	public:
		template<typename ... Us>
		auto pipeline() {
			return Pipeline<Registry, Us...>{ *this };
		}

		template<typename U>
		pool_reference_t<U> pool() {
			return std::get<pool_t<U>>(m_pools);
		}

		template<typename ... Us, typename ... Inc_Ts, typename ... Exc_Ts>
		auto view(ExcludeFilter<Exc_Ts...> exclude = ExcludeFilter<>{}, IncludeFilter<Inc_Ts... > include = IncludeFilter<>{}) {
			return View<Pipeline, GetFilter<Us...>, ExcludeFilter<Exc_Ts...>, IncludeFilter<Inc_Ts...>>{ *this };
		}

	private:
		pool_collection_t m_pools;
	};
}