#pragma once
#include "Entity.h"

namespace Gawr::ECS {
	template<typename ... Ts>
	class Registry {
	public:
		struct Filters;

		class HandleManager;
		
		template<typename U> 
		class Storage;
		
		template<typename U> 
		using Pool = std::conditional_t<std::is_same_v<std::remove_const_t<U>, Entity>, HandleManager, Storage<std::remove_const_t<U>>>;

		template<typename ... Us> 
		class Pipeline;

	private:
		using storage_collection_t = std::tuple<HandleManager, Storage<Ts>...>;

		template<typename U>
		using pool_reference_t = std::conditional_t<std::is_const_v<U>, const Pool<std::remove_const_t<U>>&, Pool<std::remove_const_t<U>>&>;
	
	public:
		template<typename ... Us>
		auto pipeline() {
			return Pipeline<Us...>{ *this };
		}

	private:
		template<typename U>
		pool_reference_t<U> pool() {
			return std::get<Pool<std::remove_const_t<U>>>(m_pools);
		}

	private:
		storage_collection_t m_pools;
	};

	
}

#include "Storage.h"
#include "HandleManager.h"
#include "Pipeline.h"
#include "View.h"
