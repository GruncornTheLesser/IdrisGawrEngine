#pragma once
#include <stdint.h>
#include <limits>
#include <vector>

namespace Gawr::ECS {

	template<typename handle_t>
	const handle_t tombstone = std::numeric_limits<handle_t>::max();

	template<bool con, typename T>
	struct conditional_member;

	template<typename T> struct conditional_member<true, T> : T { };
	template<typename T> struct conditional_member<false, T> { };

	template<typename handle_t, typename comp_t>
	class Pool {
		using comp_collection_t = conditional_member<!std::is_empty_v<comp_t>, std::vector<comp_t>>;

	public:
		Pool() : m_sparse(8, tombstone<handle_t>) { }

		handle_t handle(size_t i) const { return m_packed[i]; }
		size_t index(handle_t e) const { return m_sparse[e]; }

		size_t size() const { return m_packed.size(); }
		auto begin() const { return m_packed.begin(); };
		auto end() const { return m_packed.end(); };

		bool has(handle_t e) const { return e < m_sparse.size() && m_sparse[e] != tombstone<handle_t>; }

		void swap_i(size_t i1, size_t i2) {
			std::swap(m_sparse[m_packed[i1]], m_sparse[m_packed[i2]]);
			std::swap(m_packed[i1], m_packed[i2]);
			if constexpr(!std::is_empty_v<comp_t>) std::swap(m_comps[i1], m_comps[i2]);
		}

		comp_t& get_i(size_t i) requires !std::is_empty_v<comp_t> { return m_comps[i]; }
		
		comp_t& get(handle_t e) requires !std::is_empty_v<comp_t> { return m_comps[index(e)]; }

		template<typename ... args_ts> requires std::is_constructible_v<comp_t, args_ts...>
		std::conditional_t<std::is_empty_v<comp_t>, void, comp_t&> emplace(handle_t e, args_ts&& ... args) {
			if (m_sparse.size() <= e)
				m_sparse.resize(m_sparse.size() << 1);

			m_sparse[e] = static_cast<uint32_t>(m_packed.size());
			m_packed.push_back(e);

			if constexpr (!std::is_empty_v<comp_t>)
			{
				m_comps.emplace_back(args...);
				return m_comps.back();
			}
			
		}
		
		void erase(handle_t e) {
			m_sparse[m_packed.back()] = m_sparse[e];

			m_packed[m_sparse[e]] = m_packed.back();
			m_packed.pop_back();

			if constexpr (!std::is_empty_v<comp_t>)
			{
				m_comps[m_sparse[e]] = m_comps.back();
				m_comps.pop_back();
			}

			m_sparse[e] = tombstone<handle_t>;
		}

		template<typename pred_t = std::less<>> requires std::predicate<pred_t, comp_t&, comp_t&>
		void sort(pred_t predicate = std::less<>{}) {
			std::vector<handle_t> temp(m_packed);

			std::sort(temp.begin(), temp.end(), [&](handle_t e1, handle_t e2) {
				return predicate(m_comps[index(e1)], m_comps[index(e2)]);
				});

			for (size_t i = 0; i < m_packed.size(); i++) {
				size_t j = index(temp[i]);
				if (i != j) swap_i(i, j);
			}
		}

		template<typename t>
		void reflect(Pool<handle_t, t>& other) {
			for (size_t i = 0; i < m_packed.size(); i++) {
				size_t j = index(other.handle(i));
				if (i != j) swap_i(i, j);
			}
		}
	private:
		comp_collection_t		m_comps;
		std::vector<handle_t>	m_packed;
		std::vector<size_t>		m_sparse;
	};
}