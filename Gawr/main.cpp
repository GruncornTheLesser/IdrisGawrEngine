#include <random>

#include "Graphics.h"
#include "Gawr/ECS.h"

struct A { int a; };
struct B { int b; };
struct C { int c; };
struct D { int d; };

int main() 
{
	using namespace Gawr::ECS;

	Registry<A, B, C, D> reg;
	{
		auto pip = reg.pipeline<const A, B, C>();
		for (auto [e, a, b, c] : pip.view<const A, B, const C>()) { }

		pip.pool<B>().reorder(std::shuffle, std::mt19937{});
		pip.pool<B>().reorder(std::sort);
	}

	Application app;
	app.run();
}
