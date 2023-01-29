#pragma once
#include "Registry.h"
#include "Filters.h" // not necessary but you probably want

#include <iostream>
namespace Gawr::ECS {
	template<typename ... comp_ts> template<typename ... filter_ts>
	class Registry<comp_ts...>::View {
	public:
		using get_policy = std::tuple_element_t<0, std::tuple<filter_ts...>>;
		struct Iterator;
		struct ReverseIterator;

		View(Registry& reg) : m_reg(reg) { }

		ReverseIterator begin() {
			return ++ReverseIterator(m_reg, m_reg.pool<get_policy::orderby>().size());
		}

		ReverseIterator end() {
			return ReverseIterator(m_reg, -1);
		}

		Iterator rbegin() {
			return ++Iterator(m_reg, -1);
		}

		Iterator rend() {
			return Iterator(m_reg, m_reg.pool<get_policy::orderby>().size());
		}

	private:
		Registry& m_reg;
	};


	template<typename ... comp_ts> template<typename ... filter_ts>
	struct Registry<comp_ts...>::View<filter_ts...>::Iterator {
	public:
		using difference_type = size_t;
		using iterator_category = std::random_access_iterator_tag;

		Iterator() = default;
		Iterator(Registry& reg, size_t i) : m_reg(reg), m_index(i) { }

		auto operator*()
		{
			entity_t e = m_reg.pool<get_policy::orderby>().entity_at(m_index);
			return get_policy::template get(m_reg, e);
		}

		Iterator& operator++() 
		{
			while (++m_index != m_reg.pool<get_policy::orderby>().size() && !valid());
			return *this;
		}

		Iterator& operator--() {
			while (--m_index != -1 && !valid());
			return *this;
		}

		// Postfix increment/decrement
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
			entity_t e = m_reg.pool<get_policy::orderby>().entity_at(m_index);
			return (filter_ts::template isValid(m_reg, e) && ...);
		}

		Registry&	m_reg;
		size_t		m_index;
	};

	template<typename ... comp_ts> template<typename ... filter_ts>
	struct Registry<comp_ts...>::View<filter_ts...>::ReverseIterator : Iterator 
	{
		ReverseIterator(Registry& Reg, size_t i) : Iterator(Reg, i) { }
		ReverseIterator& operator++() {
			Iterator::operator--();
			return *this;
		}
		ReverseIterator& operator--() {
			Iterator::operator++();
			return *this;
		}
		ReverseIterator operator++(int) {
			Iterator temp = *this;
			++(*this); return temp;
		}
		ReverseIterator operator--(int) {
			Iterator temp = *this;
			--(*this); return temp;
		}
	};
}