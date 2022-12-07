#pragma once
#include <queue>
#include <vector>
#include <stdint.h>

namespace Gawr::ECS {
	class HandleManager {
	public:
		uint32_t create() {
			uint32_t h;
			if (!m_queue.empty()) {
				h = m_queue.top();
				m_queue.pop();
			}
			else
			{
				h = m_next_handle++;
			}
			return h;
		}

		void destroy(uint32_t h) {
			if (h != m_next_handle - 1)
				m_queue.push(h);
			else
				m_next_handle = h;
		}

	private:
		std::priority_queue<uint32_t> m_queue;
		uint32_t m_next_handle = 0;
	};
}