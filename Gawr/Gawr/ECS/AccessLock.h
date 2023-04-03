#pragma once
#include <shared_mutex>
namespace Gawr::ECS {

	class AccessLock {

		template<typename ... Reg_ts>
		template<typename ... Pip_ts>
		friend class Gawr::ECS::Registry<Reg_ts...>::Pipeline;
	
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

		mutable std::shared_mutex m_mtx;
	};

}
