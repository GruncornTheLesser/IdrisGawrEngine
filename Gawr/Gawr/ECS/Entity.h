#pragma once
#include <stdint.h>

namespace Gawr::ECS {
	/// @brief an uint ID to represent a collection of unique components
	using Entity = uint32_t;
	constexpr Entity tombstone = std::numeric_limits<Entity>::max();
}