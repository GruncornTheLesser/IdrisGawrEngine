#include <random>
#include <iostream>

#include "Gawr/Components/Hierarchy.h"
#include "Graphics.h"
#include "Gawr/Scene.h"
//#include "Gawr/Core/Context.h"
//#include "Gawr/Core/Window.h"

struct A { int a; };
struct B { int b; };
struct C { int c; };
struct D { int d; };



int main() 
{
	using namespace Gawr::ECS;
	
	Registry<A, B, C, D> reg;
	auto pip = reg.pipeline<Entity>();
	auto& entityPool = pip.pool<Entity>();
	entityPool.create();
	entityPool.create();
	entityPool.create();
	entityPool.erase(0);
	entityPool.erase(1);
	entityPool.erase(2);
	entityPool.create();
	entityPool.create();
	entityPool.create();


	Application app;
	app.run();
}
