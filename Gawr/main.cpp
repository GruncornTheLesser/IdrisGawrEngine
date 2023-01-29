#include <array>
#include <algorithm>
#include <concepts>
#include <iostream>
#include <mutex>
#include <typeindex>
#include <functional>

#include "Gawr/Application.h"
//#include "Gawr/ECS/Registry.h"

//using namespace Gawr::ECS;

namespace v2 {
	// header
	using entity_t = uint32_t;
	constexpr uint32_t tombstone = std::numeric_limits<uint32_t>::max();

	class registry;

	template<typename ... comp_ts>
	class typed_registry;
	class type_erased_registry;

	// view filters
	template<typename ... get_arg_ts>
	struct get_filter { };

	template<typename ... inc_ts>
	struct inc_filter { };

	template<typename ... exc_ts>
	struct exc_filter { };

	template<typename get_t, typename include_t, typename exclude_t>
	class view;

	template<typename ... args>
	class dispatcher;

	template<typename comp_t>
	class pool;

	template<typename ... arg_ts>
	class dispatcher {
	public:
		using event_t = void(*)(arg_ts...);

		void connect(event_t func) {
			auto it = std::find(m_storage.begin(), m_storage.end(), func);

			if (it == m_storage.end())
				m_storage.push_back(func);
		}

		void disconnect(event_t func) {
			auto it = std::find(m_storage.begin(), m_storage.end(), func);

			if (it != m_storage.end())
				m_storage.erase(it);
		}
		void dispatch(arg_ts ... args)
		{
			for (event_t func : m_storage)
				func(args...);
		}

	private:
		std::vector<event_t> m_storage;
	};

	template<typename comp_t>
	class pool {
	public:
		struct reference : std::reference_wrapper<pool>, std::lock_guard<std::mutex> {
			reference(pool* p)
				: std::reference_wrapper<pool>(p), 
				  std::lock_guard<std::mutex>(p.m_mutex) 
			{ }
		};
		using const_reference = const pool&;

		using sparse_t = std::vector<size_t>;
		using packed_t = std::vector<entity_t>;
		using packed_comp_t = std::vector<comp_t>;
		using get_return_t = std::conditional_t<std::is_empty_v<comp_t>, void, comp_t&>;
		using dispatcher_t = dispatcher<registry&, entity_t>;

		pool(registry& reg) : m_sparse(8, tombstone), m_reg(reg) { }

		size_t size() const {
			return m_packed.size();
		}

		entity_t at(size_t i) const {
			return m_packed[i];
		}

		size_t index(entity_t e) const {
			return m_sparse[e];
		}

		bool contains(entity_t e) const {
			return e < m_sparse.size() && m_sparse[e] != tombstone;
		}

		comp_t& get_component(entity_t e) requires (!std::is_empty_v<comp_t>)
		{
			return m_comps[m_sparse[e]];
		}

		template<typename ... args_ts> requires std::is_constructible_v<comp_t, args_ts...>
		get_return_t emplace(entity_t e, args_ts&& ... args) {
			if (m_sparse.size() <= e)
				m_sparse.resize(e, tombstone);

			m_sparse[e] = static_cast<uint32_t>(m_packed.size());
			m_packed.push_back(e);

			if constexpr (!std::is_empty_v<comp_t>)
			{
				m_comps.emplace_back(args...);
				return m_comps.back();
			}
		}

		template<typename ... arg_ts> requires std::is_constructible_v<comp_t, arg_ts...>
		get_return_t get_or_emplace(entity_t e, arg_ts&& ... args)
		{
			if constexpr (std::is_empty_v<comp_t>)
			{
				if (!contains(e)) emplace(e, args...);
			}
			else
			{
				return contains(e) ? get_component(e) : emplace(e, args...);
			}
		}

		void swap(entity_t e1, entity_t e2) {
			size_t i1 = m_sparse[e2], i2 = m_sparse[e1];
			std::swap(m_sparse[e1], m_sparse[e2]);
			std::swap(m_packed[i1], m_packed[i2]);
			if constexpr (!std::is_empty_v<comp_t>) std::swap(m_comps[i1], m_comps[i2]);

		}

		void erase(size_t i) {
			if constexpr (!std::is_empty_v<comp_t>)
			{
				m_comps[i] = m_comps.back();
				m_comps.pop_back();
			}

			m_sparse[m_packed[i]] = tombstone;
			m_sparse[m_packed.back()] = i;

			m_packed[i] = m_packed.back();
			m_packed.pop_back();
		}

		void remove(entity_t e) {
			erase(index(e));
		}


		/// <summary>
		/// executes function that reorders the collection and then updates sparse lookup data
		/// eg 
		/// reorder(std::reverse);
		/// reorder(std::sort, [](entity_t lhs, entity_t rhs) { return lhs < rhs; });
		/// reorder(std::shuffle, rng);
		/// 
		/// </summary>
		/// <param name="func">a function that takes 2 iterator parameters for begin and end the optional arg parameters</param>
		/// <param name="...args">optional arg parameters are appended after iterators</param>
		template<typename ... arg_ts>
		void reorder(void(*func)(std::vector<entity_t>::iterator, std::vector<entity_t>::iterator, arg_ts...), arg_ts&& ... args) {
			
			func(m_packed.begin(), m_packed.end(), args...);

			for (size_t pos = 0; pos < m_packed.size(); pos++)
			{

				// e1 = packed[pos]
				// curr = index
				size_t curr = m_sparse[m_packed[pos]];	// index that curr entity used to be stored at

				while (curr != pos) // if curr == next component in the correct place
				{
					size_t next = m_sparse[m_packed[curr]];
					
					if constexpr (!std::is_empty_v<comp_t>) 
						std::swap(m_comps[curr], m_comps[next]);

					m_sparse[m_packed[curr]] = curr;


					curr = next;
				};

				m_sparse[m_packed[pos]] = pos;
			}
		}

		dispatcher_t& onCreate() {
			return m_onCreate;
		}

		dispatcher_t& onDestroy() {
			return m_onDestroy;
		}

		dispatcher_t& onUpdate() {
			return m_onUpdate;
		}

	private:
		packed_comp_t m_comps;
		sparse_t m_sparse;
		packed_t m_packed;

		std::mutex m_mutex;

		registry& m_reg;

		dispatcher_t m_onCreate;
		dispatcher_t m_onDestroy;
		dispatcher_t m_onUpdate;
	};

	class registry {
	private:
		template<typename t>
		struct type_erased_deleter : std::function<void(void*)> {
			type_erased_deleter() : std::function<void(void*)>{ [](void* ptr) { delete reinterpret_cast<t*>(ptr); } } { }
		};

		template<typename t>
		using typed_ptr = std::unique_ptr<t, type_erased_deleter<t>>;
		using type_erased_ptr = std::unique_ptr<void, std::function<void(void*)>>;
		
		template<typename t>
		using pool_reference_t = std::conditional_t<std::is_const_v<t>, typename pool<std::remove_cvref_t<t>>::const_reference, typename pool<std::remove_cvref_t<t>>::reference>;

		using pool_collection_t = std::unordered_map<std::type_index, type_erased_ptr>;

	public:
		registry() = default;

		registry(const registry&) = delete;
		const registry& operator=(const registry&) = delete;

		template<typename t> 
		v2::pool<t>& pool() {
			auto key = std::type_index(typeid(std::remove_cvref_t<t>));
			auto it = m_pools.find(key);

			if (it == m_pools.end())
				it = m_pools.emplace(key, typed_ptr<v2::pool<t>>(new v2::pool<std::remove_cvref_t<t>>(*this))).first;

			return *reinterpret_cast<v2::pool<std::remove_cvref_t<t>>*>(it->second.get());
		}

		template<typename ... get_ts, typename ... inc_ts, typename ... exc_ts> 
			requires (!(std::is_reference_v<get_ts> && ...) && !(std::is_pointer_v<get_ts> && ...))
			auto view(inc_filter<inc_ts...> = inc_filter<>{}, exc_filter<exc_ts...> exclude = exc_filter<>{})
		{
			return v2::view<get_filter<get_ts...>,
				inc_filter<std::remove_cv_t<get_ts>..., std::remove_cv_t<inc_ts>...>,
				exc_filter<std::remove_cv_t<exc_ts>...>>(*this);
		}

	private:
		pool_collection_t m_pools;
	};

	template<typename ... get_arg_ts, typename ... inc_arg_ts, typename ... exc_arg_ts>
	class view<get_filter<get_arg_ts...>, inc_filter<inc_arg_ts...>, exc_filter<exc_arg_ts...>> {

		using pool_collection_t = std::tuple<pool<inc_arg_ts>&..., pool<exc_arg_ts>&...>;

		using return_t = decltype(std::tuple_cat(std::tuple<entity_t>{}, std::declval<std::conditional_t<std::is_empty_v<std::remove_cv_t<get_arg_ts>>, std::tuple<>, std::tuple<get_arg_ts&>>>()...));

		using orderby_t = pool<std::remove_cv_t<std::tuple_element_t<0, std::tuple<get_arg_ts...>>>>&;

	public:
		class Iterator {
		public:
			Iterator(view& view, size_t index) : m_view(view), m_index(index) { }

			auto operator*()
			{
				return m_view[m_index];
			}

			Iterator& operator++()
			{
				m_view.next(m_index);
				return *this;
			}

			Iterator& operator--() {
				m_view.prev(m_index);
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
			view& m_view;
			size_t m_index;
		};

		template<typename ... locked_ts>
		view(registry& reg) : m_pools(reg.pool<inc_arg_ts>()..., reg.pool<exc_arg_ts>()...) { }

		view(const view&) = delete;
		const view& operator=(const view&) = delete;

		view(view&&) = delete;
		const view& operator=(view&&) = delete;

		return_t operator[](int i) {
			entity_t e = std::get<orderby_t>(m_pools).at(i);
			return std::tuple<entity_t, get_arg_ts&...>(e, std::get<pool<std::remove_cv_t<get_arg_ts>>&>(m_pools).get_component(e)...);
		}

		Iterator begin() {
			size_t i = std::get<orderby_t>(m_pools).size();
			next(i);
			return Iterator(*this, i);
		}

		Iterator end() {
			size_t i = -1;
			prev(i);
			return Iterator(*this, i - 1);
		}

		void next(size_t& i) const {
			while (--i != -1 && !valid(i)) {}
		}

		void prev(size_t& i) const {
			while (++i != std::get<orderby_t>(m_pools).size() && !valid(i)) {}
		}

		bool valid(size_t i) const {
			entity_t e = std::get<orderby_t>(m_pools).at(i);
			return (std::get<pool<inc_arg_ts>&>(m_pools).contains(e) && ...) &&
				!(std::get<pool<exc_arg_ts>&>(m_pools).contains(e) || ...);
		}

	private:
		pool_collection_t m_pools;
	};

	



}

#include <thread>
#include <type_traits>
#include <random>


using namespace v2;

struct A { 
	void stuff() { }; 
	void const_stuff() const {}; 
	int data; 
};

struct B { int b; };
struct C { };


void shuffle(std::vector<entity_t>::iterator begin, std::vector<entity_t>::iterator end, std::mt19937 rng) {
	std::shuffle(begin, end, rng);
}


int main() 
{
	registry reg;
	reg.pool<A>().emplace(0, 0);
	reg.pool<A>().emplace(1, 1);
	reg.pool<A>().emplace(2, 2);
	reg.pool<A>().emplace(3, 3);
	reg.pool<A>().emplace(4, 4);
	reg.pool<A>().emplace(5, 5);
	reg.pool<A>().emplace(6, 6);

	std::mt19937 rng{};
	reg.pool<A>().reorder(std::shuffle, rng);

	for (auto [e, a] : reg.view<const A>()) std::cout << a.data;
	std::cout << std::endl;
	
	reg.pool<A>().reorder(std::reverse);

	for (auto [e, a] : reg.view<const A>()) std::cout << a.data;
	std::cout << std::endl;
	
	reg.pool<A>().reorder(std::sort, [](entity_t e1, entity_t e2) { return e1 < e2; });

	for (auto [e, a] : reg.view<const A>()) std::cout << a.data;
	std::cout << std::endl;

	
	//packed.push_back(0);
		
	
	//for (uint32_t i = 0; i < 5; i++) reg.pool<A>().emplace(i, i);

	//for (auto [e, a] : reg.view<const A>()) { std::cout << a.data; }

	//for (auto [e, a, b] : reg.view<const A, B>()) {}

	
	


	//Dispatcher<A> dis;
	//dis.connect([](A a) {});

	//Application().run();
}
