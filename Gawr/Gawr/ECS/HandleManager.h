#pragma once
#include "AccessLock.h"
#include <set>

namespace Gawr::ECS {
	template<typename ... Reg_Ts>
	class Registry<Reg_Ts...>::HandleManager : public AccessLock {
		template<bool reverse>
		class Iterator {
		public:
			Iterator(Entity value) : m_value(value) { }

			auto operator*() {
				return m_value;
			}
			Iterator& operator++() {
				if constexpr (reverse) --m_value;
				else				   ++m_value; 
				return *this;
			}
			
			friend bool operator==(const Iterator<reverse>& lhs, const Iterator<reverse>& rhs) {
				return lhs.m_value == rhs.m_value;
			};
			friend bool operator!=(const Iterator<reverse>& lhs, const Iterator<reverse>& rhs) {
				return lhs.m_value != rhs.m_value;
			};
		private:
			Entity m_value;
		};

	public:
		using ForwardIterator = Iterator<false>;
		using ReverseIterator = Iterator<true>;

		Entity create() {
			Entity e;
			if (!m_queue.empty()) {
				auto it = m_queue.begin();
				e = *it;
				m_queue.erase(it);
			}
			else
			{
				e = m_next++;
			}

			m_isValid.set(e);
			return e;
		}

		void destroy(Entity e) {
			m_queue.emplace(e);
			m_isValid.unset(e);
		}

		bool valid(Entity e) const {
			return m_isValid.test(e);
		}

		auto begin() const {
			return ForwardIterator(0);
		}

		auto end() const {
			return ForwardIterator(m_next);
		}

		auto rbegin() const {
			return ReverseIterator(m_next - 1);
		}

		auto rend() const {
			return ReverseIterator(-1);
		}


		size_t size() const {
			return m_next - m_queue.size();
		}

	private:
		struct bitset {
			void set(size_t index) {
				size_t i = index / 32, j = index % 32;
				if (i >= m_data.size()) m_data.resize(i + 1);
				m_data[i] |= (1 >> j);
			}

			void unset(size_t index) {
				size_t i = index / 32, j = index % 32;
				if (i >= m_data.size()) m_data.resize(i + 1);
				m_data[i] &= ~(1 >> j);
			}

			size_t size() const {
				return 32 * m_data.size();
			}

			bool test(size_t index) const {
				size_t i = index / 32, j = index % 32;
				return m_data[i] & (1 >> j);
			}

			std::vector<size_t> m_data;
		} m_isValid;
		std::set<Entity> m_queue;
		Entity m_next;
	};
}
