#pragma once
#include <vector>
#include <stdint.h>
#include <limits>
#include <queue>

using std::uint32_t;

namespace Gawr {
	namespace ECS {
		typedef uint32_t entity_t;
		const entity_t tombstone = std::numeric_limits<entity_t>::max();
		
		class handle_manager {
		public:
			entity_t create() {
				entity_t e;
				if (!m_queue.empty()) {
					e = m_queue.top();
					m_queue.pop();
				}
				else
				{
					e = m_next_e;
					m_next_e++;
				}
				return e;
			}
			void destroy(entity_t e) {
				if (e != m_next_e - 1)
					m_queue.push(e);
				else
					m_next_e = e;
			}

		private:
			std::priority_queue<entity_t> m_queue;
			uint32_t m_next_e = 0;
		};

		template<typename comp_t>
		class basic_storage {
		public:
			basic_storage(size_t size = 256) : m_sparse(size, tombstone), m_packed() {
				m_packed.reserve(size);
			}

			entity_t operator[](size_t index) const {
				return m_packed[index];
			}

			void reserve(size_t size) {
				if (m_packed.size() >= size) return;

				m_sparse.resize(size, tombstone);
				m_packed.reserve(size);
			}

			template<typename ... args_ts>
			comp_t& emplace(entity_t e, args_ts&& ... args) {
				m_sparse[e] = static_cast<uint32_t>(m_packed.size());
				m_packed.push_back(e);
				m_comps.emplace_back(args...);
				return m_comps.back();
			}

			void erase(entity_t e) {
				m_sparse[m_packed.back()] = m_sparse[e];

				m_packed[m_sparse[e]] = m_packed.back();
				m_packed.pop_back();

				m_comps[m_sparse[e]] = m_comps.back();
				m_comps.pop_back();

				m_sparse[e] = tombstone;
			}

			void swap(size_t i1, size_t i2) {
				std::swap(m_sparse[m_packed[i1]], m_sparse[m_packed[i2]]);
				std::swap(m_packed[i1], m_packed[i2]);
				std::swap(m_comps[i1], m_comps[i2]);
			}

			void clear() {
				std::for_each(m_packed.begin(), m_packed.end(), [&](entity_t& e) { m_sparse[e] = tombstone; });
				std::fill(m_sparse.begin(), m_sparse.end(), tombstone);
				m_packed.clear();
				m_comps.clear();
			}

			template<typename predicate_t = std::less<>>
			void sort(predicate_t predicate = std::less<>{}) {
				std::sort(m_packed.begin(), m_packed.end(), [&](entity_t e1, entity_t e2) {
					return predicate(m_comps[index_of(e1)], m_comps[index_of(e2)]);
					});

				std::vector<comp_t> comps(m_comps.size());
				for (int i = 0; i < m_packed.size(); i++) {
					comps[i] = std::move(m_comps[index_of(m_packed[i])]);
					m_sparse[m_packed[i]] = i;
				}

				m_comps = comps;
			}

			template<typename t>
			void reflect(basic_storage<t>& other) {
				std::sort(m_packed.begin(), m_packed.end(), [&](entity_t e1, entity_t e2) {
					return other.index_of(e1) < other.index_of(e2);
					});

				std::vector<comp_t> comps(m_comps.size());
				for (int i = 0; i < m_packed.size(); i++) {
					comps[i] = std::move(m_comps[index_of(m_packed[i])]);
					m_sparse[m_packed[i]] = i;
				}

				m_comps = comps;
			}

			bool has(entity_t e) const {
				return e < m_sparse.size() && m_sparse[e] != tombstone;
			}

			size_t index_of(entity_t e) const {
				return m_sparse[e];
			}

			comp_t& get_component(size_t i) {
				return m_comps[i];
			}

			size_t size() const {
				return m_packed.size();
			}

			auto begin() const { return m_packed.begin(); };

			auto end() const { return m_packed.end(); };

		private:
			std::vector<size_t>		m_sparse;	// sparsely paged array of index to entities
			std::vector<entity_t>	m_packed;	// packed array of ordered entities
			std::vector<comp_t>		m_comps;	// packed array of components
		};

		template<typename ... comp_ts>
		class basic_registry : public handle_manager, private basic_storage<comp_ts>... {
		public:
			template<typename view_comp1_t, typename ... view_comp_ts>
			class basic_view {
			public:
				struct Iterator {
					using value_type = std::tuple<entity_t, view_comp1_t, view_comp_ts...>;
					using difference_type = uint32_t;
					using reference = std::tuple<entity_t, view_comp1_t&, view_comp_ts&...>;
					using pointer = std::tuple<entity_t, view_comp1_t*, view_comp_ts*...>;
					using iterator_category = std::random_access_iterator_tag;

					Iterator() = default;
					Iterator(basic_registry<comp_ts...>* reg, size_t i) : m_reg(reg), m_index(i) { }
					
					reference operator*() {
						entity_t e = m_reg->storage<view_comp1_t>()[m_index];
						return std::tuple<entity_t, view_comp1_t&, view_comp_ts&...>(e,
							m_reg->storage<view_comp1_t>().get_component(m_index),
							m_reg->get<view_comp_ts>(e) ...);
					}
					
					pointer operator->() {
						entity_t e = m_reg->storage<view_comp1_t>()[m_index];
						return std::tuple<entity_t, view_comp1_t*, view_comp_ts*...>(e,
							&m_reg->storage<view_comp1_t>().get_component(m_index),
							&m_reg->get<view_comp_ts>(e) ...);
					}

					// Prefix increment
					Iterator& operator++() {
						do { m_index++; } 
						while (m_index < m_reg->storage<view_comp1_t>().size() && !m_reg->has<view_comp1_t, view_comp_ts...>(m_reg->storage<view_comp1_t>()[m_index]));
						
						return *this; 
					}
					Iterator operator++(int) { 
						Iterator temp = *this;
						++(*this); 
						return temp; 
					}

					// Prefix decrement
					Iterator& operator--() { 
						do { m_index--; } 
						while (m_index > 0 && m_reg->has<view_comp1_t, view_comp_ts...>(m_reg->storage<view_comp1_t>()[m_index]));
						
						return *this; 
					}
					Iterator operator--(int) {
						Iterator temp = *this;
						--(*this); 
						return temp;
					}

					friend bool operator==(const Iterator& a, const Iterator& b) { 
						return a.m_index == b.m_index;
					};
					friend bool operator!=(const Iterator& a, const Iterator& b) {
						return a.m_index != b.m_index; 
					};

				private:
					basic_registry<comp_ts...>*	m_reg;
					size_t						m_index;
				};
				
				basic_view(basic_registry* reg) : m_reg(reg) { }

				Iterator begin() { return ++Iterator(m_reg, -1); };
				Iterator end() { return Iterator(m_reg, m_reg->storage<view_comp1_t>().size()); };

				Iterator rbegin() { return --Iterator(m_reg, m_reg->storage<view_comp1_t>().size()); };
				Iterator rend() { return Iterator(m_reg, -1); };
			
			private:
				basic_registry* m_reg;
			};

			template<typename ... ts>
			basic_view<ts...> view() {
				return basic_view<ts...>{ this };
			}

			template<typename t>
			basic_storage<t>& storage() {
				return static_cast<basic_storage<t>&>(*this);
			}

			template<typename ... ts>
			bool has(entity_t e) const {
				return (basic_storage<ts>::has(e) && ...);
			}

			template<typename t> 
			t& get(entity_t e) {
				return basic_storage<t>::get_component(basic_storage<t>::index_of(e));
			}

			template<typename t>
			t* try_get(entity_t e) {
				return basic_storage<t>::has(e) ? &basic_storage<t>::get_component(e) : nullptr;
			}
		
			template<typename t, typename ... args_ts>
			t& emplace(entity_t e, args_ts&& ... args) {
				return basic_storage<t>::emplace(e, args...);
			}
		};
	}
}
