#pragma once
#include "AccessLock.h"
#include <set>

namespace Gawr::ECS {
	class HandleManager : public AccessLock {
	private:
		struct node { std::ptrdiff_t prev, next; };
	
	public:
		struct Iterator {
		public:
			Iterator(const HandleManager& m, Entity e)
				: m_node(&m.m_nodes[0] + e), m_curr(e) 
			{ }

			Entity operator*() {
				return m_curr;
			}
			Iterator& operator++() {
				if (m_node->next == 0) {
					m_curr = std::numeric_limits<uint32_t>::max();
					m_node = nullptr;
				}
				else
				{
					m_curr += m_node->next;
					m_node += m_node->next;
				}
				
				return *this;
			}
			bool operator!=(const Iterator& other) const {
				return m_curr != other.m_curr;
			}
		private:
			Entity		m_curr;
			const node* m_node;
		};
		
		Entity create() {
			uint32_t e;
			node* curr;

			if (m_end != std::numeric_limits<uint32_t>::max())		// if invalid list not empty
			{
				// pop off the back of invalid list
				e = m_end;		
				curr = &m_nodes[m_end];

				// if invalid list now empty
				if (curr->next == 0)
					m_end = std::numeric_limits<uint32_t>::max();	// mark invalid list empty
				else
					m_end = curr + curr->next - &m_nodes[0];		// get next in invalid list
			}
			else
			{
				// append to sparse array
				e = m_nodes.size();
				curr = &m_nodes.emplace_back();
			}
			
			if (m_begin != std::numeric_limits<uint32_t>::max()) // if valid list empty
			{
				// [..., begin, curr], curr is new beginning
				node* begin = &m_nodes[m_begin];
				begin->prev = curr - begin;		// offset from prev to curr
				curr->next = begin - curr;		// offset from curr to prev
			}
			else
			{
				curr->prev = 0;
				curr->next = 0;
			}

			m_begin = curr - &m_nodes[0];	// set begin to curr
			return e;
		}

		void erase(Entity e) {
			node* prev, *curr = &m_nodes[e], *next;

			uint32_t neighbor_case = 0;
			if (curr->next != 0) neighbor_case |= 1;
			if (curr->prev != 0) neighbor_case |= 2;

			switch (neighbor_case)
			{
			case 0: // [X]
				m_begin = std::numeric_limits<uint32_t>::max();
				break;

			case 1: // [X, next, ...]
				next = curr + curr->next;
				m_begin = next - &m_nodes[0];
				next->prev = 0;
				break;

			case 2: // [..., prev, X]
				prev = curr + curr->prev;
				prev->next = 0;
				break;

			case 3: // [..., prev, X, next, ...]
				next = curr + curr->next;
				prev = curr + curr->prev;

				next->prev = prev - next;	// offset from next to prev
				prev->next = next - prev;	// offset from prev to next
				break;
			}
			
			if (m_end != std::numeric_limits<uint32_t>::max())	// if invalid list not empty
				curr->next = &m_nodes[m_end] - curr;			// add link to front of invalid list
			else
				curr->next = 0;						

			curr->prev = std::numeric_limits<std::ptrdiff_t>::max();	// mark as invalid
			m_end = e;													// set as beginning of invalid list
		}

		void update(Entity e) {
			if (e == m_begin) // already front of list
				return; 

			node* curr = &m_nodes[e];
			node* next = curr + curr->next;
			node* prev = curr + curr->prev;
			
			if (curr->next == 0) {
				prev = curr + curr->prev;
				prev->next = 0;
			}
			else {
				next = curr + curr->next;
				prev = curr + curr->prev;

				next->prev = prev - next;	// offset from next to prev
				prev->next = next - prev;	// offset from prev to next
			}

			node* begin = &m_nodes[m_begin];
			next->prev = curr - begin;		// offset from begin to curr
			curr->next = begin - curr;		// offset from curr to begin

			m_begin = e;
		}
		
		bool valid(Entity e) const {
			return (e < m_nodes.size() && m_nodes[e].prev != std::numeric_limits<std::ptrdiff_t>::max());
		}

		Iterator begin() const {
			return Iterator(*this, m_begin);
		}
		
		Iterator end() const {
			return Iterator(*this, std::numeric_limits<uint32_t>::max());
		}

	private:
		std::vector<node> m_nodes;
		uint32_t m_begin{ std::numeric_limits<uint32_t>::max() };
		uint32_t m_end{ std::numeric_limits<uint32_t>::max() };
	};
}
