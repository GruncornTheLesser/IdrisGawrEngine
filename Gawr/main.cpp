#include <random>
#include <iostream>

//#include "Gawr/Components/Hierarchy.h"
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
	
	// iterate over view
	for (auto& a : pip.query<Select<A>, From<Entity>, AllOf<A>>()) { }	// SELECT A SortBy Entity

	for (auto [a, b] : pip.query<Select<const A, const B>, From<Entity>, AllOf<A>>()) { 
		const A& a_ = a;

	}
	for (auto e : pip.query<Select<Entity>, From<Entity>>()) { }			// SELECT Entity SortBy Entity
	for (auto [a, b] : pip.query<Select<A, B>, From<A>>()) { }			// SELECT A, B SortBy A
	for (auto [a, b] : pip.query<Select<A, B>, From<A>>()) { }			// SELECT A, B SortBy A

	//for (auto [a, b] : pip.view<A, B>()) { }

	Application app;
	app.run();
}
