#include <random>
#include <iostream>

#include "Graphics.h"
#include "Gawr/ECS.h"

struct A { int a; };
struct B { int b; };
struct C { int c; };
struct D { int d; };

int main() 
{
	using namespace Gawr::ECS;

	Application app;
	app.run();
}
