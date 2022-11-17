#include "Gawr/Application.h"
#include "Graphics.h"
#include "ecs.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <typeindex>

typedef int A;
typedef float B;

using namespace Gawr::ECS;

int main() 
{
	/*
	{
		
		A a = 1;
		B b = 2;

		basic_registry<A, B> reg;

		entity_t e0 = reg.create();
		entity_t e1 = reg.create();
		entity_t e2 = reg.create();
		entity_t e3 = reg.create();
		entity_t e4 = reg.create();

		reg.emplace<A>(e0, 2);	// 0, 2		// 3, 0
		reg.emplace<A>(e1, 3);	// 1, 3		// 2, 1
		reg.emplace<A>(e2, 1);	// 2, 1		// 0, 2
		reg.emplace<A>(e3, 0);	// 3, 0		// 1, 3
		reg.emplace<A>(e4, 4);	// 4, 4		// 4, 4
		reg.storage<A>().sort();

		reg.emplace<B>(e0, 2.0f);	// 0, 2		// 3, 0
		reg.emplace<B>(e1, 3.0f);	// 1, 3		// 2, 1
		reg.emplace<B>(e2, 1.0f);	// 2, 1		// 0, 2
		reg.emplace<B>(e3, 0.0f);	// 3, 0		// 1, 3
		reg.emplace<B>(e4, 4.0f);	// 4, 4		// 4, 4
		reg.storage<B>().reflect(reg.storage<A>());
	
		for (auto [e, a, b] : reg.view<A, B>())
			std::cout << e << ", " << a << ", " << b << std::endl;
	}
	*/
	Application graphics;
	//Gawr::Application app;
}

