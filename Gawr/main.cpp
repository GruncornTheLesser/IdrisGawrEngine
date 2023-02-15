#include <array>
#include <algorithm>
#include <concepts>
#include <iostream>
#include <mutex>
#include <typeindex>
#include <functional>
#include <shared_mutex>
#include <random>

#include "Gawr/Application.h"

//#include "Gawr/ECS/Registry.h"

//using namespace Gawr::ECS;

namespace v2 {
	// forward declarations

	/// @brief an uint ID to represent a collection of unique components
	using Entity = uint32_t;
	
	/// @brief an ordered set of entities with a sparse set lookup and a corresponding component
	/// @tparam the component type
	template<typename T>
	class Pool;
	
	/// @brief a pipeline represents a reference to a registry. 
	/// @tparam Reg_T - the registry type
	/// @tparam Ts - defines the component read write access. eg 'const T' or 'T'
	template<typename Reg_T,  typename ... Ts>
	class Pipeline;

	/// @brief an inlude filter.
	/// @tparam ...Ts the components that must be present to validate an entity
	template<typename ... Ts>
	struct IncludeFilter;

	/// @brief an exclude filter.
	/// @tparam ...Ts  the components that must be absent to validate an entity
	template<typename ... Ts>
	struct ExcludeFilter;

	/// @brief a get filter. Types are automatically included and retrieved when iterating in a view
	/// @tparam ...Ts the components are retrieved from the entity 
	template<typename ... Ts>
	struct GetFilter;

	template<typename Reg_T, typename ... Filter_Ts>
	class View;

	template<typename T>
	class Pool {
		template<typename Reg_T, typename ... Us>
		friend class Pipeline; // for the lock/unlock business
		
		template<typename ... Arg_Ts>
		using reorder_func_t = void(*)(std::vector<Entity>::iterator, std::vector<Entity>::iterator, Arg_Ts&&...);

		using get_return_t = std::conditional_t<std::is_empty_v<T>, void, T&>;

		static constexpr auto tombstone = std::numeric_limits<uint32_t>::max();

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
				if constexpr (!std::is_empty_v<T>)
				{
					return m_components[m_sparse[e]] = T(std::forward<Arg_Ts>(args)...);
				}
				else
				{
					return;
				}
			}
			else
			{
				if (m_sparse.size() <= e)
					m_sparse.resize(e + 1, tombstone);

				m_sparse[e] = m_packed.size();
				m_packed.push_back(e);

				if constexpr (!std::is_empty_v<T>)
				{
					m_components.emplace_back(args...);
					return m_components.back();
				}
			}
		}

		void swap(Entity e1, Entity e2) {
			size_t i1 = m_sparse[e2], i2 = m_sparse[e1];

			std::swap(m_sparse[e1], m_sparse[e2]);
			std::swap(m_packed[i1], m_packed[i2]);

			if constexpr (!std::is_empty_v<T>) std::swap(m_components[i1], m_components[i2]);
		}

		void erase(size_t i) {
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

			if (std::is_empty_v<T>)
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
		void lock() { m_mtx.lock(); }
		void unlock() { m_mtx.unlock(); }

		void lock() const { m_mtx.lock_shared(); }
		void unlock() const { m_mtx.unlock_shared(); }

		std::vector<T>			m_components;
		std::vector<size_t>		m_sparse;
		std::vector<Entity>		m_packed;

		mutable std::shared_mutex m_mtx;
	};

	template<typename ... Ts>
	class Registry {
		using pool_collection_t = std::tuple<Pool<std::remove_const_t<Ts>>...>;

		template<typename U>
		using pool_t = Pool<std::remove_const_t<U>>;

		template<typename U>
		using pool_reference_t = std::conditional_t<std::is_const_v<U>, const Pool<std::remove_const_t<U>>&, Pool<std::remove_const_t<U>>&>;

	public:
		template<typename U>
		pool_reference_t<U> pool() {
			return std::get<pool_t<U>>(m_pools);
		}

		template<typename ... Us>
		auto pipeline() {
			return Pipeline<Registry, Us...>{ *this };
		}

		template<typename ... Us, typename ... Inc_Ts, typename ... Exc_Ts>
		auto view(ExcludeFilter<Exc_Ts...> exclude = ExcludeFilter<>{}, IncludeFilter<Inc_Ts... > include = IncludeFilter<>{}) {
			return View<Pipeline, GetFilter<Us...>, ExcludeFilter<Exc_Ts...>, IncludeFilter<Inc_Ts...>>{ *this };
		}

	private:
		pool_collection_t m_pools;
	};

	template<typename ... Ts>
	struct IncludeFilter {
		template<typename Reg_T>
		static bool valid(Reg_T& reg, Entity e) {
			return (reg.pool<Ts>().contains(e) && ...);
		}
	};

	template<typename ... Ts>
	struct ExcludeFilter {
		template<typename Reg_T>
		static bool valid(Reg_T& reg, Entity e) {
			return !(reg.template pool<Ts>().contains(e) || ...);
		}
	};

	template<typename ... Ts>
	struct GetFilter : IncludeFilter<Ts...> {
		using orderby_t = std::tuple_element_t<0, std::tuple<Ts...>>;

		template<typename Reg_T> 
		static auto retrieve(Reg_T& reg, Entity e) {
			return std::tuple_cat(std::tuple(e), [&]<typename Comp_T>()->auto
			{
				if constexpr (!std::is_empty_v<Comp_T>)
					return std::tuple<Comp_T&>{ reg.template pool<Comp_T>().getComponent(e) };
				else
					return std::tuple<>{};
			}.operator()<Ts>() ...);
		}
	};



	template<typename Registry_T, typename ... Filter_Ts>
	class View {
		using RetrieveFilter = std::tuple_element_t<0, std::tuple<Filter_Ts...>>;
		using orderby_t = RetrieveFilter::orderby_t;
	public:
		class Iterator {
		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using difference_type = std::ptrdiff_t;

			Iterator(Registry_T& reg, size_t index)
				: m_reg(reg), m_index(index) { }

			auto operator*() {
				Entity e = m_reg.template pool<orderby_t>().at(m_index);
				return RetrieveFilter::retrieve(m_reg, e);
			}

			Iterator& operator--() {
				while (++m_index != m_reg.template pool<orderby_t>().size() && !valid());
				return *this;
			}
			Iterator& operator++() {
				while (--m_index != -1 && !valid());
				return *this;
			}

			//Postfix increment / decrement
			Iterator operator++(int) {
				Iterator temp = *this;
				++(*this);
				return temp;
			}

			Iterator operator--(int) {
				Iterator temp = *this;
				--(*this);
				return temp;
			}

			friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
				return lhs.m_index == rhs.m_index;
			};
			friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
				return lhs.m_index != rhs.m_index;
			};

		private:
			bool valid() {
				Entity e = m_reg.pool<orderby_t>().at(m_index);
				return (Filter_Ts::valid(m_reg, e) && ...);
			}

			Registry_T& m_reg;
			size_t m_index;
		};

		View(Registry_T& reg) : m_reg(reg) { }

		Iterator begin() {
			return ++Iterator{ m_reg, m_reg.pool<orderby_t>().size() };
		}

		Iterator end() {
			return Iterator{ m_reg, static_cast<size_t>(-1) };
		}

	private:
		Registry_T& m_reg;
	};

	template<template<typename...> typename Reg_T, typename ... Reg_Ts, typename ... Ts>
	class Pipeline<Reg_T<Reg_Ts...>, Ts...> {
		template<typename U>
		using pool_reference_t = std::conditional_t<std::is_const_v<U>, const Pool<std::remove_const_t<U>>&, Pool<std::remove_const_t<U>>&>;

		template<typename U>
		static bool constexpr stored_as_non_const = (std::is_same_v<std::remove_const_t<U>, Ts> || ...);

		template<typename U>
		static bool constexpr stored_as_const = (std::is_same_v<const std::remove_const_t<U>, Ts> || ...);

		template<typename U>
		static bool constexpr stored = (std::is_same_v<std::remove_const_t<U>, std::remove_const_t<Ts>> || ...);

	public:
		Pipeline(Reg_T<Reg_Ts...>& reg) : m_reg(reg)
		{
			// for each type in reg -> orderedby registry so consistent locking order
			([&]<typename U>()->void
			{
				// if in non const pipeline
				if constexpr ((std::is_same_v<U, Ts> || ...))
					m_reg.template pool<U>().lock();

				// if const in pipeline
				if constexpr ((std::is_same_v<const U, Ts> || ...))
					m_reg.template pool<const U>().lock();

			}.operator()<Reg_Ts>(), ...);
			
		}

		~Pipeline() {
			// for each type in pipeline
			(m_reg.template pool<Ts>().unlock(), ...);
		}
		
		template<typename U>
		auto& pool() {
			static_assert(stored_as_non_const<U> || (stored_as_const<U> && std::is_const_v<U>), 
				"requested pool is not managed by this pipeline");

			return m_reg.template pool<U>();			
		}

		template<typename ... Us, typename ... Inc_Ts, typename ... Exc_Ts>
		auto view(ExcludeFilter<Exc_Ts...> exclude = ExcludeFilter<>{}, IncludeFilter<Inc_Ts... > include = IncludeFilter<>{}) {
			static_assert(((stored_as_non_const<Us> || (stored_as_const<Us> && std::is_const_v<Us>)) && ...),
				"requested get component/s in view is not accessible by this pipeline");
			static_assert((stored<Inc_Ts> && ...), 
				"requested include component/s in view is not accessible by this pipeline");
			static_assert((stored<Exc_Ts> && ...),
				"requested exclude component/s in view is not accessible by this pipeline");

			return View<Pipeline, GetFilter<Us...>, ExcludeFilter<Exc_Ts...>, IncludeFilter<Inc_Ts...>>{ *this };
		}

		template<typename ... Us>
		auto pipeline() {
			static_assert((stored_as_const<Us> && ...), 
				"cannot create sub-pipeline using non const component");

			return Pipeline<Pipeline<Reg_T<Reg_Ts...>, Ts...>, Us...>{ *this };
		}

	private:
		Reg_T<Reg_Ts...>& m_reg;
	};
}

namespace testing {
	using namespace v2;

	struct A {
		void stuff() { };
		void const_stuff() const {};
		int a;
	};

	struct B {
		void stuff() { };
		void const_stuff() const {};
		int b;
	};
	struct C { };

	
	void read(Registry<A, B, C>& reg, std::string name) {
		auto pipline = reg.pipeline<const A>();
		auto& pool = pipline.pool<const A>();

		std::cout << name << " read open\n";
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		std::cout << name << " read close\n";
	}

	void write(Registry<A, B, C>& reg, std::string name) {
		auto pipline = reg.pipeline<A>();
		auto& pool = pipline.pool<A>();

		std::cout << name << " write open\n";
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		std::cout << name << " write close\n";
	}
}


int main() 
{
	using namespace testing;

	Registry<A, B, C> reg;
	{
		auto pip1 = reg.pipeline<const A, B>();

		//Pool<A>& p1 =		pip1.pool<A>();
		const Pool<A>& p2 = pip1.pool<const A>();
		const Pool<B>& p3 = pip1.pool<const B>();
		Pool<B>& p4 =		pip1.pool<B>();


		for (auto [e, a, b] : pip1.view<const A, B>(ExcludeFilter<B>{}))
		{
			a.const_stuff();
			b.const_stuff();
			b.stuff();
		}



		auto& p = reg.pool<A>();
		for (int i = 0; i < 20; i++)
			p.emplace(i, i);

	}

	std::thread t1(read, std::ref(reg), "t1");
	std::thread t2(read, std::ref(reg), "t2");
	std::thread t3(write, std::ref(reg), "t3");
	std::thread t4(write, std::ref(reg), "t4");

	std::thread t5(read, std::ref(reg), "t5");
	std::thread t6(write, std::ref(reg), "t6");
	std::thread t7(read, std::ref(reg), "t7");
	
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	t6.join();
	t7.join();
	
}
