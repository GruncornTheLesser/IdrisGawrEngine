#pragma once
#include <stdint.h>
#include <limits>

namespace Gawr::ECS {
	using entity_t = uint32_t;
	constexpr entity_t tombstone = std::numeric_limits<entity_t>::max();
}