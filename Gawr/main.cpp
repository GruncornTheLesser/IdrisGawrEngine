#include "Gawr/Application.h"
#include "Graphics.h"
#include "Gawr/ECS/Registry.h"
#include "Gawr/ECS/EventDispatcher.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <typeindex>
#include <iostream>

using A = int;
using B = float;
struct C {
	C() { std::cout << "I WAS INITIALIZED" << std::endl; }
	C(const C& other) { std::cout << "I WAS COPIED >:(" << std::endl; }
};

template <bool B, typename T>
struct member;

template <typename T>
struct member<true, T> : std::vector<T> { };

template <typename T>
struct member<false, T> { };

template<bool con>
struct D {
	void conTrue() requires con { }
	void conFalse() requires !con { }
	
	member<con, int> data;
};

int main() 
{
	D<true> d;
	d.data.emplace_back(0);

	//Gawr::ECS::Pool<uint32_t, C> puh;
	//puh.emplace(1);
	//puh.emplace(2);
	//puh.erase(1);

	//test.create<A>();
	//test.get<A>();

	{
		using namespace Gawr::ECS;
		Registry<uint32_t, A, B, C> reg;

		uint32_t e0 = reg.create();
		uint32_t e1 = reg.create();
		uint32_t e2 = reg.create();
		uint32_t e3 = reg.create();
		uint32_t e4 = reg.create();
		
		reg.emplace<C>(e0);
		reg.emplace<C>(e1);
		reg.emplace<C>(e2);
		reg.emplace<C>(e3);
		reg.emplace<C>(e4);
		
		reg.emplace<A>(e0, 2);	// 0, 2		// 3, 0
		reg.emplace<A>(e1, 3);	// 1, 3		// 2, 1
		reg.emplace<A>(e2, 1);	// 2, 1		// 0, 2
		reg.emplace<A>(e3, 0);	// 3, 0		// 1, 3
		reg.emplace<A>(e4, 4);	// 4, 4		// 4, 4

		reg.pool<A>().sort();
		//reg.pool<C>().sort([](handle_t e1, handle_t e2) { return e1 > e2; };);

		reg.emplace<B>(e0, 2.0f);	// 0, 2		// 3, 0
		reg.emplace<B>(e1, 3.0f);	// 1, 3		// 2, 1
		reg.emplace<B>(e2, 1.0f);	// 2, 1		// 0, 2
		reg.emplace<B>(e3, 0.0f);	// 3, 0		// 1, 3
		reg.emplace<B>(e4, 4.0f);	// 4, 4		// 4, 4

		reg.pool<B>().reflect(reg.pool<A>());

		auto start = reg.view<A, B>().begin();
		auto end = reg.view<A, B>().end();
		
		auto rstart = reg.view<A, B>().rbegin();
		auto rend = reg.view<A, B>().rend();

		auto& view = reg.view<A, B>();

		for (auto [e, a, b] : reg.view<A, B>()) {
			//std::cout << reg.pool<A>().index(e) << std::endl;
			if (e == 2) reg.erase<A>(e);
		}
	}
	
	Application test_implementation;
	//Gawr::Application app;
}
