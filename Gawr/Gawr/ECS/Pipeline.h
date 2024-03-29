#pragma once
#include "Storage.h"
#include "HandleManager.h"
#include "Filters.h"

namespace Gawr::ECS {	
	/// @brief an access manager for multithreaded applications. 
	/// @tparam ...Ts the access managed component eg 'const T' or 'T'
	template<typename...Reg_Ts>
	template<typename ... Ts>
	class Registry<Reg_Ts...>::Pipeline {
		template<typename U>
		using pool_reference_t = std::conditional_t<std::is_const_v<U>, 
			const Pool<std::remove_const_t<U>>&,	// const pool
			Pool<U>&>;								// write pool

		template<typename U>
		static bool constexpr stored_as_non_const = (std::is_same_v<std::remove_const_t<U>, Ts> || ...);

		template<typename U>
		static bool constexpr stored_as_const = (std::is_same_v<const std::remove_const_t<U>, Ts> || ...);

		template<typename U>
		static bool constexpr stored = (std::is_same_v<std::remove_const_t<U>, std::remove_const_t<Ts>> || ...);

	public:
		template<typename Select, typename From, typename Where>
		class View;

	public:
		Pipeline(Registry& reg) : m_reg(reg)
		{
			// for each type in reg -> orderedby registry so consistent locking order
			([&]<typename U>()->void
			{
				if constexpr (stored_as_non_const<U>)
					m_reg.pool<U>().lock();

				else if constexpr (stored_as_const<U>)
					m_reg.pool<const U>().lock();

			}.operator()<Reg_Ts>(), ...);
		}

		~Pipeline() {
			// unlock all, order doesnt matter
			(m_reg.pool<Ts>().unlock(), ...);
		}

		Pipeline(const Pipeline&) = delete;
		Pipeline& operator=(const Pipeline&) = delete;
	
		template<typename U>
		auto& pool() {
			static_assert(stored_as_non_const<U> || (stored_as_const<U> && std::is_const_v<U>),
				"requested pool cannot be acquired by this pipeline");

			return m_reg.template pool<U>();
		}

		template<typename Select_T, 
			typename From_T = internal::DefaultFrom<Select_T>::type, 
			typename Where_T = internal::DefaultWhere<Select_T, From_T>::type>
		auto view()
		{
			return View<Select_T, From_T, Where_T>{ *this };
		}
	private:
		Registry& m_reg;
	};
}


