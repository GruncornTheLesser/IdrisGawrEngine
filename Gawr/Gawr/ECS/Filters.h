#pragma once
namespace Gawr::ECS {
	template<template<typename ...> typename Filter_T, typename Tuple_T>
	struct ToFilter;

	template<template<typename ...> typename Filter_T, typename ... Ts>
	struct ToFilter<Filter_T, std::tuple<Ts...>> { using type = Filter_T<Ts...>; };

	template<typename ... Us>
	struct Include {
		template<typename Pip_T>
		static bool call(Pip_T& pip, Entity e) {
			if constexpr (sizeof...(Us) == 0) return true;
			return (pip.pool<const Us>().contains(e) && ...);
		}
	};

	template<typename ... Us>
	struct Exclude {
		template<typename Pip_T>
		static bool call(Pip_T& pip, Entity e) {
			if constexpr (sizeof...(Us) == 0) return true;
			return !(pip.pool<const Us>().contains(e) || ...);
		}
	};

	template<typename U>
	struct SortBy {
		using type = U;
	};

	template<typename ... Us>
	struct Retrieve {
		static_assert(!(std::is_empty_v<Us> || ...), "Empty types are not stored and cannot be retrieved.");

	private:
		using U1 = std::tuple_element_t<0, std::tuple<Us...>>;
	public:
		template<typename Pip_T>
		static auto call(Pip_T& pip, Entity e) {
			return std::tuple_cat([&]<typename U>()->auto
			{
				if constexpr (std::is_same_v<U, Entity>) 
					return std::tuple(e);
				else
					return std::tuple<U&>{ pip.pool<U>().getComponent(e) };
			}.operator()<Us>()...);
		}

		template<typename Pip_T>
		static void call(Pip_T& pip, Entity e) requires (sizeof...(Us) == 0) {
			return;
		}

		template<typename Pip_T>
		static Entity call(Pip_T& pip, Entity e) requires (sizeof...(Us) == 1 && std::is_same_v<U1, Entity>) {
			return e;
		}

		template<typename Pip_T>
		static auto& call(Pip_T& pip, Entity e) requires (sizeof...(Us) == 1 && !std::is_same_v<U1, Entity>) {
			return pip.pool<U1>().getComponent(e);
		}

		template<typename Pip_T>
		using Return_T = decltype(call<Pip_T>(std::declval<Pip_T&>(), std::declval<Entity>()));

		using Filter_T = ToFilter<Include, decltype(
			std::tuple_cat(std::declval<std::conditional_t<std::is_same_v<Entity, Us>, std::tuple<>, std::tuple<Us>>>()...))>::type;
		using DefaultSortBy_T = SortBy<U1>;
	};
}
