#pragma once
#include "Pool.h"
#include "View.h"

namespace Gawr::ECS {
	/// @brief an access manager for multithreaded applications. 
	/// @tparam Manager_T the parent registry or pipeline
	/// @tparam ...Ts the access managed component eg 'const T' or 'T'
	template<typename Manager_T, typename ... Ts>
	class Pipeline;
	
	template<template<typename...> typename Manager_T, typename ... Reg_Ts, typename ... Ts>
	class Pipeline<Manager_T<Reg_Ts...>, Ts...> {
		template<typename U>
		using pool_reference_t = std::conditional_t<std::is_const_v<U>, const Pool<std::remove_const_t<U>>&, Pool<std::remove_const_t<U>>&>;

		template<typename U>
		static bool constexpr stored_as_non_const = (std::is_same_v<std::remove_const_t<U>, Ts> || ...);

		template<typename U>
		static bool constexpr stored_as_const = (std::is_same_v<const std::remove_const_t<U>, Ts> || ...);

		template<typename U>
		static bool constexpr stored = (std::is_same_v<std::remove_const_t<U>, std::remove_const_t<Ts>> || ...);

	public:
		Pipeline(Manager_T<Reg_Ts...>& reg) : m_reg(reg)
		{
			// for each type in reg -> orderedby registry so consistent locking order
			([&]<typename U>()->void
			{
				// if in non const pipeline
				if constexpr ((std::is_same_v<U, Ts> || ...))
					m_reg.template pool<U>().lock();

				// if const in pipeline
				if constexpr ((std::is_same_v<const U, Ts> || ...))
					m_reg.template pool<const U>().lock();

			}.operator() < Reg_Ts > (), ...);

		}

		~Pipeline() {
			// for each type in pipeline
			(m_reg.template pool<Ts>().unlock(), ...);
		}

		template<typename U>
		auto& pool() {
			static_assert(stored_as_non_const<U> || (stored_as_const<U> && std::is_const_v<U>),
				"requested pool is not managed by this pipeline");

			return m_reg.template pool<U>();
		}

		template<typename ... Us, typename ... Inc_Ts, typename ... Exc_Ts>
		auto view(ExcludeFilter<Exc_Ts...> exclude = ExcludeFilter<>{}, IncludeFilter<Inc_Ts... > include = IncludeFilter<>{}) {

			static_assert((stored<Inc_Ts> && ...),
				"requested include component/s in view is not accessible by this pipeline");
			static_assert((stored<Exc_Ts> && ...),
				"requested exclude component/s in view is not accessible by this pipeline");

			return View<Pipeline, GetFilter<Us...>, ExcludeFilter<Exc_Ts...>, IncludeFilter<Inc_Ts...>>{ *this };
		}

		template<typename ... Us>
		auto pipeline() {
			static_assert((stored_as_const<Us> && ...),
				"cannot create sub-pipeline using non const component");

			return Pipeline<Pipeline<Reg_T<Reg_Ts...>, Ts...>, Us...>{ *this };
		}

	private:
		Manager_T<Reg_Ts...>& m_reg;
	};

}
