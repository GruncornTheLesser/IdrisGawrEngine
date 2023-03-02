#pragma once
#include "Entity.h"
#include<tuple>

namespace Gawr::ECS {

	/// @brief a class to allow iteration over a entities that match the query. 
	/// @tparam ...Filter_Ts entities must pass all filter arguments. the first is used to retrieve entities components.
	template<typename ... Reg_Ts>
	template<typename ... Pip_Ts>
	template<typename OrderBy_T, typename Retrieve_T, typename ... Filters_Ts>
	class Registry<Reg_Ts...>::Pipeline<Pip_Ts...>::View {

	public:
		template<typename pool_iterator_t>
		class Iterator {
		public:
			Iterator(Pipeline& pip, pool_iterator_t begin, pool_iterator_t end)
				: m_pipeline(pip), m_current(begin), m_end(end) 
			{
				while (m_current != m_end && !valid()) ++m_current;
			}

			Retrieve_T::template Return_T<Pipeline<Pip_Ts...>> operator*() {
				return Retrieve_T::call(m_pipeline, *m_current);
			}

			Iterator& operator++() {
				while (++m_current != m_end && !valid());
				return *this;
			}

			//Postfix increment / decrement
			Iterator operator++(int) {
				Iterator temp = *this;
				++(*this);
				return temp;
			}

			friend bool operator==(const Iterator<pool_iterator_t>& lhs, const Iterator<pool_iterator_t>& rhs) {
				return lhs.m_current == rhs.m_current;
			};
			friend bool operator!=(const Iterator<pool_iterator_t>& lhs, const Iterator<pool_iterator_t>& rhs) {
				return lhs.m_current != rhs.m_current;
			};

		private:

			bool valid() {
				if constexpr (std::is_same_v<OrderBy_T, Entity>) {
					if (!m_pipeline.pool<const Entity>().valid(*m_current)) return false;
				}
				return (Filters_Ts::call(m_pipeline, *m_current) && ...);
			}

			Pipeline&		m_pipeline;
			pool_iterator_t m_current;
			pool_iterator_t m_end;
		};
		using ForwardIterator = Iterator<typename Pool<OrderBy_T>::ForwardIterator>;
		using ReverseIterator = Iterator<typename Pool<OrderBy_T>::ReverseIterator>;

		View(Pipeline<Pip_Ts...>& pip) : m_pipeline(pip) { }

		auto begin() const {
			auto& pool = m_pipeline.pool<const OrderBy_T>();
			return ForwardIterator(m_pipeline, pool.begin(), pool.end());
		}

		auto end() const {
			auto& pool = m_pipeline.pool<const OrderBy_T>();
			return ForwardIterator(m_pipeline, pool.end(), pool.end());
		}

		auto rbegin() const {
			auto& pool = m_pipeline.pool<const OrderBy_T>();
			return ReverseIterator(m_pipeline, pool.rbegin(), pool.rend());
		}

		auto rend() const {
			auto& pool = m_pipeline.pool<const OrderBy_T>();
			return ReverseIterator(m_pipeline, pool.rend(), pool.rend());
		}


	private:
		Pipeline<Pip_Ts...>& m_pipeline;
	};
}
