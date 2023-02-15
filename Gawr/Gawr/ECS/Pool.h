#pragma once
#include "Entity.h"

#include <vector>
#include <shared_mutex>

namespace Gawr::ECS {
	template<typename T>
	class Pool {
		template<typename Reg_T, typename ... Us>
		friend class Pipeline; // for the lock/unlock business

		template<typename ... Arg_Ts>
		using reorder_func_t = void(*)(std::vector<Entity>::iterator, std::vector<Entity>::iterator, Arg_Ts&&...);

		using get_return_t = std::conditional_t<std::is_empty_v<T>, void, T&>;

		static constexpr auto tombstone = static_cast<uint32_t>(-1);

	public:
		Pool() : m_sparse(8, tombstone) { }

		size_t size() const {
			return m_packed.size();
		}

		Entity at(size_t i) const {
			return m_packed[i];
		}

		size_t index(Entity e) const {
			return m_sparse[e];
		}

		bool contains(Entity e) const {
			return e < m_sparse.size() && m_sparse[e] != tombstone;
		}

		T& getComponent(Entity e) requires (!std::is_empty_v<T>) {
			return m_components[m_sparse[e]];
		}

		const T& getComponent(Entity e) const requires (!std::is_empty_v<T>) {
			return m_components[m_sparse[e]];
		}

		template<typename ... Arg_Ts>
		get_return_t emplace(Entity e, Arg_Ts&& ... args) {
			if (contains(e))
			{
				if constexpr (std::is_empty_v<T>) return;
				
				return m_components[m_sparse[e]] = T(std::forward<Arg_Ts>(args)...);
			}
			else
			{
				if (m_sparse.size() <= e)
					m_sparse.resize(e + 1, tombstone);

				m_sparse[e] = m_packed.size();
				m_packed.push_back(e);

				if constexpr (std::is_empty_v<T>) return;

				return m_components.emplace_back(args...);
			}
		}

		void swap(Entity e1, Entity e2) {
			size_t i1 = m_sparse[e2], i2 = m_sparse[e1];

			std::swap(m_sparse[e1], m_sparse[e2]);
			std::swap(m_packed[i1], m_packed[i2]);

			if constexpr (!std::is_empty_v<T>) std::swap(m_components[i1], m_components[i2]);
		}

		void erase(size_t i) {
			// swap and pop policy
			
			if constexpr (!std::is_empty_v<T>)
			{
				m_components[i] = m_components.back();
				m_components.pop_back();
			}

			m_sparse[m_packed[i]] = tombstone;
			m_sparse[m_packed.back()] = i;

			m_packed[i] = m_packed.back();
			m_packed.pop_back();
		}

		void remove(Entity e) {
			erase(index(e));
		}

		template<typename ... Arg_Ts>
		void reorder(reorder_func_t<Arg_Ts...> func, Arg_Ts&& ... args) {

			func(m_packed.begin(), m_packed.end(), std::forward<Arg_Ts>(args)...);

			if constexpr (std::is_empty_v<T>)
			{
				for (size_t i = 0; i < m_packed.size(); i++)				// iterate through the packed to update the lookup in the sparse array
					m_sparse[m_packed[i]] = i;
			}
			else
			{
				for (size_t pos = 0; pos < m_packed.size(); pos++)			// iterate through starting index
				{
					size_t curr = m_sparse[m_packed[pos]];					// index that entity at pos used to be stored at

					while (curr != pos)										// if curr == pos, component is in the correct index
					{
						size_t next = m_sparse[m_packed[curr]];				// index that entity at curr used to be stored at

						m_sparse[m_packed[curr]] = curr;					// set sparse lookup for curr to the new index
						std::swap(m_components[curr], m_components[next]);	// move component at curr to correct position

						curr = next;										// set curr to next
					}

					m_sparse[m_packed[curr]] = pos;							// when I remove this it breaks. must be important.
				}
			}
		}

	private:
		void lock() { 
			m_mtx.lock(); 
		}
		void unlock() { 
			m_mtx.unlock();
		}

		void lock() const { 
			m_mtx.lock_shared(); 
		}
		void unlock() const { 
			m_mtx.unlock_shared(); 
		}

		std::vector<T>			m_components;
		std::vector<size_t>		m_sparse;
		std::vector<Entity>		m_packed;

		mutable std::shared_mutex m_mtx;
};
}