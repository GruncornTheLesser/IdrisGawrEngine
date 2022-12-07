#pragma once
namespace Gawr::ECS {
	template<typename reg_t, typename t, typename ... ts>
	class View final : private reg_t {
	public:
		View() = delete;

		auto begin() {
			return ++ReverseIterator(*this, reg_t::template pool<t>().size());
		}

		auto end() {
			return ReverseIterator(*this, -1);
		}

		auto rbegin() {
			return ++Iterator(*this, -1);
		}

		auto rend() {
			return Iterator(*this, reg_t::template pool<t>().size());
		}

		struct Iterator {
			using value_type = std::tuple<uint32_t, t, ts...>;
			using difference_type = uint32_t;
			using reference = std::tuple<uint32_t, t&, ts&...>;
			using pointer = std::tuple<uint32_t, t*, ts*...>;
			using iterator_category = std::random_access_iterator_tag;

			Iterator() = default;
			Iterator(reg_t& Reg, size_t i) : m_reg(Reg), m_index(i) { }

			reference operator*() {
				uint32_t e = m_reg.template pool<t>().handle(m_index);
				return std::tuple<uint32_t, t&, ts&...>(e,
					m_reg.template pool<t>().get_i(m_index),
					m_reg.template get_i<ts>(e) ...);
			}

			// Prefix increment/decrement
			Iterator& operator++() {
				while (++m_index < m_reg.template pool<t>().size() &&
					!m_reg.template has<ts...>(m_reg.template pool<t>().handle(m_index)));
				return *this;
			}
			Iterator& operator--() {
				while (--m_index > -1 &&
					!m_reg.template has<ts...>(m_reg.template pool<t>().handle(m_index)));
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
			reg_t& m_reg;
			size_t m_index;
		};

		struct ReverseIterator : public Iterator {
			ReverseIterator(reg_t& Reg, size_t index)
				: Iterator(Reg, index)
			{ }
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
	};
}