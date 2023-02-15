#pragma once
#include "Entity.h"
#include<tuple>

namespace Gawr::ECS {
	template<typename ... Ts>
	struct IncludeFilter {
		template<typename Reg_T>
		static bool valid(Reg_T& reg, Entity e) {
			return (reg.template pool<Ts>().contains(e) && ...);
		}
	};

	template<typename ... Ts>
	struct ExcludeFilter {
		template<typename Reg_T>
		static bool valid(Reg_T& reg, Entity e) {
			return !(reg.template pool<Ts>().contains(e) || ...);
		}
	};

	template<typename ... Ts>
	struct GetFilter : IncludeFilter<Ts...> {
		using orderby_t = std::tuple_element_t<0, std::tuple<Ts...>>;

		template<typename Reg_T>
		static auto retrieve(Reg_T& reg, Entity e) {
			return std::tuple_cat(std::tuple(e), [&]<typename Comp_T>()->auto
			{
				if constexpr (!std::is_empty_v<Comp_T>)
					return std::tuple<Comp_T&>{ reg.template pool<Comp_T>().getComponent(e) };
				else
					return std::tuple<>{};
			}.operator() < Ts > () ...);
		}
	};

	template<typename Manager_T, typename ... Filter_Ts>
	class View {
		using RetrieveFilter = std::tuple_element_t<0, std::tuple<Filter_Ts...>>;
		using orderby_t = RetrieveFilter::orderby_t;
	public:
		class Iterator {
		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using difference_type = std::ptrdiff_t;

			Iterator(Manager_T& reg, size_t index)
				: m_reg(reg), m_index(index) { }

			auto operator*() {
				Entity e = m_reg.template pool<orderby_t>().at(m_index);
				return RetrieveFilter::retrieve(m_reg, e);
			}

			Iterator& operator--() {
				while (++m_index != m_reg.template pool<orderby_t>().size() && !valid());
				return *this;
			}
			Iterator& operator++() {
				while (--m_index != -1 && !valid());
				return *this;
			}

			//Postfix increment / decrement
			Iterator operator++(int) {
				Iterator temp = *this;
				++(*this);
				return temp;
			}

			Iterator operator--(int) {
				Iterator temp = *this;
				--(*this);
				return temp;
			}

			friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
				return lhs.m_index == rhs.m_index;
			};
			friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
				return lhs.m_index != rhs.m_index;
			};

		private:
			bool valid() {
				Entity e = m_reg.template pool<orderby_t>().at(m_index);
				return (Filter_Ts::valid(m_reg, e) && ...);
			}

			Manager_T& m_reg;
			size_t m_index;
		};

		View(Manager_T& reg) : m_reg(reg) { }

		Iterator begin() {
			return ++Iterator{ m_reg, m_reg.template pool<orderby_t>().size() };
		}

		Iterator end() {
			return Iterator{ m_reg, static_cast<size_t>(-1) };
		}

	private:
		Manager_T& m_reg;
	};
}
