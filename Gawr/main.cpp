#include <random>
#include <iostream>

#include "Gawr/Components/Hierarchy.h"

#include "Graphics.h"
#include "Gawr/Scene.h"


struct A { int a; };
struct B { int b; };
struct C { int c; };
struct D { int d; };

int main() 
{
	using namespace Gawr::ECS;
	Registry<A, B, C, D> reg; 

	auto pip = reg.pipeline<Entity, A, B, const C>();
	
	auto& hm = pip.pool<Entity>();
	

	auto e0 = hm.create();
	auto e1 = hm.create();
	auto e2 = hm.create();
	hm.destroy(e1);
	//auto e4 = hm.create();

	auto& pl = pip.pool<A>();
	pl.emplace(e0, 0);
	pl.emplace(e1, 0);
	pl.emplace(e2, 0);



	for (auto it = hm.begin(), end = hm.end(); it != end; ++it) {
		std::cout << *it << '\n';
	}

	using a_t = Retrieve<A>::Return_T<Registry<A, B, C>::Pipeline<A>>;
	using e_t = Retrieve<Entity>::Return_T<Registry<A, B, C>::Pipeline<Entity>>;
	using ab_t = Retrieve<A, B>::Return_T<Registry<A, B, C>::Pipeline<A, B>>;
	using ac_t = Retrieve<A, const C>::Return_T<Registry<A, B, C>::Pipeline<A, C>>;


	// iterate over view
	for (auto& a : pip.view<Entity, A>()) { }						// SELECT A SortBy Entity
	for (auto& a : pip.view<Entity, const A>()) { }					// SELECT const A SortBy Entity
	for (auto e : pip.view<Entity, Entity>()) { }					// SELECT Entity SortBy Entity
	for (auto [a, b] : pip.view<A, A, B>()) { }						// SELECT A, B SortBy A
	for (auto [a, b] : pip.view<B, A, const B>()) { }				// SELECT A, const B SortBy B

	// iterate over pool
	for (auto it = pl.begin(), end = pl.end(); it != end; ++it) { }
	for (auto it = pl.rbegin(), end = pl.rend(); it != end; ++it) { }
	for (auto it = hm.begin(), end = hm.end(); it != end; ++it) { }
	for (auto it = hm.rbegin(), end = hm.rend(); it != end; ++it) { }

	Application app;
	app.run();
}
