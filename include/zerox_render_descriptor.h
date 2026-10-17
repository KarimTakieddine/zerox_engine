#pragma once

#include <cstdint>

namespace ZeroX
{
	struct alignas(16) EntityFetchDescriptor
	{
		uint64_t stride{ 0 };
		uint64_t offset{ 0 };
	};

	enum class EntitySectionType : uint64_t
	{
		UNKNOWN		= 0,
		BACKGROUND	= 1,
		SPRITE		= 2,
		COLLIDER	= 3,
		PARTICLE	= 4,
		TEXT		= 5
		// etc...
	};

	enum class EntitySectionFlags : uint64_t
	{
		NONE = 0
		// etc...
	};

	struct alignas(16) EntitySection
	{
		EntitySectionType type					{ EntitySectionType::UNKNOWN };
		EntitySectionFlags flags				{ EntitySectionFlags::NONE };
		EntityFetchDescriptor* fetchDescriptors	{ nullptr };
		uint64_t descriptorCount				{ 0 };
	};

	struct alignas(16) RenderDescriptor
	{
		const std::byte* entitySectionData	{ nullptr };
		uint64_t sectionCount				{ 0 };
	};
}
