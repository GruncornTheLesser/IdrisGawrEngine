#pragma once
#include "Entity.h"
#include<tuple>

namespace Gawr::ECS {
	template<typename ... Ts>
	struct IncludeFilter {
		using IndexBy_t = std::tuple_element_t<0, std::tuple<Ts...>>;

		template<typename Reg_T>
		static auto retrieve(Reg_T& reg, Entity e) {
			return e;
		}
		template<typename Reg_T>
		static bool valid(Reg_T& reg, Entity e) {
			return (reg.template pool<Ts>().contains(e) && ...);
		}
	};

	template<>
	struct IncludeFilter<> { 
		// Include filter with no args cannot be used to retrieve
		template<typename Reg_T> 
		static bool valid(Reg_T&, Entity) { 
			return true; 
		}
	};

	template<typename ... Ts>
	struct ExcludeFilter {
		// Exclude filter cannot be used to retrieve

		template<typename Reg_T>
		static bool valid(Reg_T& reg, Entity e) {
			return !(reg.template pool<Ts>().contains(e) || ...);
		}
	};

	template<typename ... Ts>
	struct GetFilter {
		
		using IndexBy_t = std::tuple_element_t<0, std::tuple<Ts...>>;

		template<typename Reg_T>
		static auto retrieve(Reg_T& reg, Entity e) {
			return std::tuple_cat(std::tuple(e), [&]<typename Comp_T>()->auto
			{
				if constexpr (!std::is_empty_v<Comp_T>)
					return std::tuple<Comp_T&>{ reg.template pool<Comp_T>().getComponent(e) };
				else
					return std::tuple<>{};
			}.operator()<Ts> () ...);
		}

		template<typename Reg_T>
		static bool valid(Reg_T& reg, Entity e) {
			return (reg.template pool<Ts>().contains(e) && ...);
		}
	};

	/// @brief a class to allow iteration over a entities that match the query. 
	/// @tparam Manager_T 
	/// @tparam ...Filter_Ts entities must pass all filter arguments. the first is used to retrieve entities components.
	template<typename Manager_T, typename ... Filter_Ts>
	class View {
		using RetrieveFilter = std::tuple_element_t<0, std::tuple<Filter_Ts...>>;
		using IndexBy_t = RetrieveFilter::IndexBy_t;
	public:
		class Iterator {
		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using difference_type = std::ptrdiff_t;

			Iterator(Manager_T& reg, size_t index)
				: m_reg(reg), m_index(index) { }

			auto operator*() {
				return RetrieveFilter::retrieve(m_reg, entity());
			}

			Iterator& operator--() {
				while (++m_index != m_reg.template pool<IndexBy_t>().size() && !valid());
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

			auto index() { return m_index; }
			auto entity() { return m_reg.template pool<IndexBy_t>().at(m_index); }

		private:
			bool valid() {
				Entity e = m_reg.template pool<IndexBy_t>().at(m_index);
				return (Filter_Ts::valid(m_reg, e) && ...);
			}

			Manager_T& m_reg;
			size_t m_index;
		};

		class ReverseIterator : public Iterator {
		public:
			ReverseIterator(Manager_T& reg, size_t i) 
				: Iterator(reg, i) { }

			ReverseIterator& operator--() {
				Iterator::operator++();
				return *this;
			}
			ReverseIterator& operator++() {
				Iterator::operator--();
				return *this;
			}

			//Postfix increment / decrement
			ReverseIterator operator++(int) {
				ReverseIterator temp = *this;
				++(*this);
				return temp;
			}
			ReverseIterator operator--(int) {
				ReverseIterator temp = *this;
				--(*this);
				return temp;
			}
		};

		View(Manager_T& reg) : m_reg(reg) { }

		Iterator begin() {
			return ++Iterator{ m_reg, m_reg.template pool<IndexBy_t>().size() };
		}

		Iterator end() {
			return Iterator{ m_reg, static_cast<size_t>(-1) };
		}

		ReverseIterator rbegin() {
			return ++ReverseIterator{ m_reg, static_cast<size_t>(-1) };
		}

		ReverseIterator rend() {
			return ReverseIterator{ m_reg, m_reg.template pool<IndexBy_t>().size() };
		}


	private:
		Manager_T& m_reg;
	};
}
