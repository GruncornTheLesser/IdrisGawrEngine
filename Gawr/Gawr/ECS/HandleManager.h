#pragma once
#include <vector>
#include "Entity.h"

namespace Gawr::ECS {

	class HandleManager {
	public:
		entity_t create() {
			entity_t h;
			if (m_queue.empty()) {
				h = m_next_entity++;
			}
			else
			{
				h = m_queue.back();
				m_queue.pop_back();
			}
			return h;
		}

		void destroy(entity_t h) {
			if (h == m_next_entity - 1)
				m_next_entity = h;
			else
				m_queue.push_back(h);
		}

		bool valid(entity_t h) {
			return h < m_next_entity && std::find(m_queue.begin(), m_queue.end(), h) == m_queue.end();
		}

	private:
		std::vector<entity_t> m_queue;
		entity_t m_next_entity = 0;
	};
}