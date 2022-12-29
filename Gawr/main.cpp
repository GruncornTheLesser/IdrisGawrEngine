#include "Gawr/Application.h"
#include "Gawr/ECS/Registry.h"
#include "Gawr/ECS/View.h"



#include <iostream>
#include <bitset>
#include <string>


using namespace Gawr::ECS;

struct A { int a; };
struct B { int b; };
struct C { };

template<typename ... ts>
struct D { };

template<typename ... ts>
struct E { };

class test : Registry<A, B, C> {
public:
	test() { 
		for (int i = 0; i < 10; i++)
			create();

		for (int i = 1; i < 9; i++)
			pool<A>().emplace(i, (i + 3) % 10);

		for (int i = 0; i < 4; i++) 
			pool<B>().emplace(i, (i + 6) % 10);

		for (int i = 6; i < 10; i++)
			pool<C>().emplace(i);
	}

	void print() {
		std::cout << "e |A |B |C " << std::endl;
		for (int e = 0; e < 10; e++)
			std::cout << e <<
			" |" << (pool<A>().has(e) ? std::to_string(pool<A>().get(e).a) : " ") <<
			" |" << (pool<B>().has(e) ? std::to_string(pool<B>().get(e).b) : " ") <<
			" |" << (pool<C>().has(e) ? "#" : " ") <<
			std::endl;
	}

	void iterator() {
		
		std::cout << "A\n";
		for (auto [e, b] : view<include<B>>())
		{
			std::cout << e << std::endl;
		}

		std::cout << "A & C & !B\n";
		for (auto [e, a] : view<include<A, C>, exclude<B>>())
		{
			std::cout << e << std::endl;
		}
		
		std::cout << "A & B\n";
		for (auto [e, a, b] : view<include<A, B>>())
		{
			std::cout << e << std::endl;
		}

		std::cout << "A & !B & !C\n";
		for (auto [e, a] : view<include<A>, exclude<B, C>>())
		{
			std::cout << e << std::endl;
		}

		struct custom_filter {
			static bool isValid(Registry& reg, entity_t e) { 
				return reg.pool<A>().get(e).a > 4; 
			}
		};

		std::cout << "A A.a > 4\n";
		for (auto [e, a] : view<include<A>, custom_filter>()) {
			std::cout << e << std::endl;
		}
	}

	void sort() {
		print();
		
		std::cout << "-----" << std::endl;
		pool<A>().sort([&](A lhs, A rhs) { return lhs.a < rhs.a; });
		
		std::cout << "e :";
		for (auto e : pool<A>()) std::cout << e << ":" << pool<A>().get(e).a << ", ";
		std::cout << std::endl;
		/*
		std::cout << "-----" << std::endl;
		pool<A>().sort([&](entity_t lhs, entity_t rhs) { return lhs < rhs; });
		
		std::cout << "e :";
		for (auto e : pool<A>()) std::cout << e << ":" << pool<A>().get(e).a << ", ";
		std::cout << std::endl;
		
		std::cout << "-----" << std::endl;

		pool<B>().sort([&](B lhs, B rhs) { return lhs.b < rhs.b; });

		std::cout << "e :";
		for (auto e : pool<B>()) std::cout << e << ":" << pool<B>().get(e).b << ", ";
		std::cout << std::endl;

		pool<B>().reflect(pool<A>());

		std::cout << "-----" << std::endl;

		std::cout << "e :";
		for (auto e : pool<B>()) std::cout << e << ":" << pool<B>().get(e).b << ", ";
		std::cout << std::endl;
		*/
	}

};

int main() 
{
	test t{};
	t.iterator();
	//t.sort();
	
}