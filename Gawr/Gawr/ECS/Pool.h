#pragma once
#include "Entity.h"
#include <vector>
#include <functional>
#include <algorithm>

namespace Gawr::ECS {
	template<typename comp_t>
	class Pool : private std::conditional_t<std::is_empty_v<comp_t>, comp_t, std::vector<comp_t>> {
		
		
		using m_comps = std::vector<comp_t>;
		using stored_t = std::conditional_t<std::is_empty_v<comp_t>, void, comp_t&>;
		using iterator_t = std::vector<entity_t>::reverse_iterator;
	public:
		Pool() : m_sparse(8, tombstone) { }

		// gets the entity stored at a given index
		entity_t entity_at(size_t i) const {
			return m_packed[i];
		}

		// gets the index of a given entity
		size_t index_of(entity_t e) const {
			return m_sparse[e];
		}

		// returns the number of elements in pool
		size_t size() const {
			return m_packed.size();
		}

		// entity iterator begin, in reverse order so stays valid when entity erased
		iterator_t begin() const {
			return m_packed.rbegin();
		};

		// entity iterator end
		iterator_t end() const {
			return m_packed.rend();
		};

		// returns true if entity is stored in pool
		bool has(entity_t e) const {
			return e < m_sparse.size() && m_sparse[e] != tombstone;
		}

		// swaps to entities by their index
		void swap_at(size_t i1, size_t i2) {
			entity_t e1 = m_packed[i1], e2 = m_packed[i2];
			std::swap(m_sparse[e1], m_sparse[e2]);
			std::swap(m_packed[i1], m_packed[i2]);
			if constexpr (!std::is_empty_v<comp_t>) std::swap(m_comps::at(i1), m_comps::at(i2));
		}

		// swaps to entities by their index
		void swap(entity_t e1, entity_t e2) {
			size_t i1 = m_sparse[e2], i2 = m_sparse[e1];
			std::swap(m_sparse[e1], m_sparse[e2]);
			std::swap(m_packed[i1], m_packed[i2]);
			if constexpr (!std::is_empty_v<comp_t>) std::swap(m_comps::at(i1), m_comps::at(i2));
		}

		// gets the component stored at index i
		comp_t& get_at(size_t i) const requires !std::is_empty_v<comp_t> {
			return m_comps::at(i);
		}

		// gets the component stored for entity e
		comp_t& get(entity_t e) const requires !std::is_empty_v<comp_t> {
			return get_at(index_of(e));
		}

		comp_t* try_get(entity_t e) const requires !std::is_empty_v<comp_t> {
			return has(e) ? get_at(index_of(e)) : nullptr;
		}

		template<typename ... arg_ts> requires std::is_constructible_v<comp_t, arg_ts...>
		stored_t get_or_emplace(entity_t e, arg_ts&& ... args) {
			if constexpr (std::is_empty_v<comp_t>) {
				if (!has(e)) emplace(e, args...);
				return;
			}
			else
			{
				return has(e) ? get_at(index_of(e)) : emplace(e, args...);
			}

		}

		// adds entity to pool, if component type is not empty assigns component to entity
		template<typename ... args_ts> requires std::is_constructible_v<comp_t, args_ts...>
		stored_t emplace(entity_t e, args_ts&& ... args) {
			if (m_sparse.size() <= e)
				m_sparse.resize(m_sparse.size() << 1, tombstone);

			m_sparse[e] = static_cast<uint32_t>(m_packed.size());
			m_packed.push_back(e);

			if constexpr (!std::is_empty_v<comp_t>)
			{
				m_comps::emplace_back(args...);
				return m_comps::back();
			}
		}

		// remove entity at index from pool
		void erase_at(size_t i) {
			if constexpr (!std::is_empty_v<comp_t>)
			{
				m_comps::at(i) = m_comps::back();
				m_comps::pop_back();
			}

			m_sparse[m_packed[i]] = tombstone;
			m_sparse[m_packed.back()] = i;

			m_packed[i] = m_packed.back();
			m_packed.pop_back();
		}

		// removes entity from pool
		void erase(entity_t e) {
			erase_at(index_of(e));
		}

		// sorts entities in pool
		void sort(std::function<bool(entity_t, entity_t)> cmp) {
			std::vector<entity_t> temp(m_packed);

			std::sort(temp.begin(), temp.end(), cmp);

			for (size_t i = 0; i < m_packed.size(); i++) {
				size_t j = index_of(temp[i]);
				if (i != j) swap_at(i, j);
			}
		}

		void sort(std::function<bool(const comp_t&, const comp_t&)> cmp) requires (!std::is_empty_v<comp_t>) {
			sort([&](entity_t lhs, entity_t rhs) { return cmp(get(lhs), get(rhs)); });
		}

		// copies the order of entities from other pool to this pool
		template<typename t>
		void reflect(Pool<t>& other) {
			for (size_t i = 0; i < m_packed.size(); i++) {
				size_t j = index_of(other.entity_at(i));
				if (j != tombstone && i != j) swap_at(i, j);
			}
		}

		uint32_t version() const {
			return m_version;
		}

	private:
		std::vector<entity_t>		m_packed;
		std::vector<size_t>			m_sparse;
		uint32_t					m_version;
	};
	template<> class Pool<void>;
}