#pragma once
#include "../Scene.h"

struct Parent {
	Gawr::ECS::Entity m_parent;
	operator Gawr::ECS::Entity() const { return m_parent; }

};

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Transform {
	struct Local {
		glm::mat4 m_matrix;
		Local(const glm::mat4& m) : m_matrix(m) { }
		operator const glm::mat4& () const { return m_matrix; }
	};
	struct World {
		glm::mat4 m_matrix;
		World(const glm::mat4& m) : m_matrix(m) { }
		operator const glm::mat4&() const { return m_matrix; }

		friend glm::mat4 operator *(const World& world, const Local& local) {
			return world.m_matrix * local.m_matrix;
		}
	};
	
	struct Position {
	public:
		operator const glm::vec3&() const { return m_position; }
		operator glm::mat4() const { return glm::translate(m_position); }
		friend glm::mat4 operator *(const Position& pos, const Local& mat) {
			return glm::translate(pos.m_position) * mat.m_matrix;
		}
	private:
		glm::vec3 m_position;
	};
	struct Rotation {
		glm::quat m_rotation;
		operator const glm::quat&() const { return m_rotation; }
		operator glm::mat4() const { return glm::mat4_cast(m_rotation); }
		friend glm::mat4 operator *(const Rotation& rot, const Local& mat) {
			return glm::mat4_cast(rot.m_rotation) * mat.m_matrix;
		}
	};
	struct Scale {
		glm::vec3 m_scale;
		operator const glm::vec3&() const { return m_scale; }
		operator glm::mat4() const { return glm::scale(m_scale); }
		friend glm::mat4 operator *(const Scale& scl, const Local& mat) {
			return glm::scale(scl.m_scale) * mat.m_matrix;
		}
	};
	
	struct WorldPosition {
	public:
		WorldPosition(const glm::vec3& position, const glm::vec3& worldScale, const glm::quat& worldRotation) {
			m_position = worldRotation * (position * worldScale) * glm::conjugate(worldRotation);
		}

		WorldPosition(const glm::vec3& parentPosition, const glm::vec3& position, const glm::vec3& worldScale, const glm::quat& worldRotation) {
			m_position = parentPosition + worldRotation * (position * worldScale) * glm::conjugate(worldRotation);
		}

		operator const glm::vec3& () const { return m_position; }
		operator glm::mat4() const { return glm::translate(m_position); }
	private:
		glm::vec3 m_position;
	};
	struct WorldRotation {
		glm::quat m_rotation;
		WorldRotation(const glm::quat& r) : m_rotation(r) { }
		operator const glm::quat& () const { return m_rotation; }
	};
	struct WorldScale {
		glm::vec3 m_scale;
		WorldScale(const glm::vec3& scl) : m_scale(scl) { }
		operator const glm::vec3& () const { return m_scale; }
	};

	struct UpdateTag {};
}

namespace Mesh {
	enum class Attrib {
		Index = -1,
		Position = 0,
		Normal = 1,
		Tangent = 2,
		TexCoord = 3,
		MaterialIndex = 4,
		// BoneID = 5,
		// BoneWeight = 6
	};

	template<Attrib>
	class VBO { };

	class VAO { };
}

struct Scene : Gawr::ECS::Registry<
	Gawr::ECS::Entity, // entity pool
	Parent, 
	Transform::Position, 
	Transform::Rotation, 
	Transform::Scale, 
	Transform::World,
	Transform::Local,
	Transform::UpdateTag
	//Mesh::VAO,
	//Mesh::VBO<Mesh::Attrib::Index>,
	//Mesh::VBO<Mesh::Attrib::Position>,
	//Mesh::VBO<Mesh::Attrib::Normal>,
	//Mesh::VBO<Mesh::Attrib::Tangent>,
	//Mesh::VBO<Mesh::Attrib::TexCoord>,
	//Mesh::VBO<Mesh::Attrib::MaterialIndex>
> { };

// Maybe: calculate transform using quaternion where possible to avoid transform calculations???
// could add an alternative to world matrix and instead use world transform object which would store a position, a quaternion and a scale
// then we individually transform using quaternion maths which should be cheaper than matrix maths
// scale = product of scalers
// rotation = sum of quaternions
// position = position + rotation * scale * local position
// then only where needed do you convert that to a matrix?
// must multithread otherwise its no faster than generic transform maths
// if I have to convert most values to matrix transforms any also likely isnt very useful
// but for bone transforms

void updateHierarchy(Scene& registry) {
	using namespace Gawr::ECS;

	auto pipeline = registry.pipeline<const Entity, Parent>();

	auto& parentPool = pipeline.pool<Parent>();
	auto& entityPool = pipeline.pool<const Entity>();

	for (auto it = parentPool.rbegin(); it != parentPool.rend();) {	// iterate over Hierarchies in reverse
		Parent& parent = parentPool.getComponent(*it);				// parent to current entity

		if (!entityPool.valid(parent)) {	// if parent invalid
			parentPool.erase(*it);			// remove parent (does not capture all error case)
			continue;						// dont iterate new component will fill space
		}
						
		if (parentPool.contains(parent))					// if parent has parent
		{
			Parent& next = parentPool.getComponent(parent);	// current entity's parent's parent 

			
			if (parentPool.index(*it) > parentPool.index(parent))	// if parent appears before next parent 
			{							
				parentPool.swap(*it, parent);						// swap their order to make parent appear first
				continue;											// dont iterate ie check parent next
			}
		}

		it++;
	}
}


// matrix technique
void updateLocalTransform(Scene& registry) {
	using namespace Gawr::ECS;
	using namespace Transform;
	
	auto pipeline = registry.pipeline<Local, const UpdateTag, const Position, const Rotation, const Scale>();

	auto& posPool = pipeline.pool<const Position>();
	auto& rotPool = pipeline.pool<const Rotation>();
	auto& sclPool = pipeline.pool<const Scale>();

	for (auto [e, local] : pipeline.view<Select<Entity, Transform::Local>, From<Transform::UpdateTag>>())
	{
		if (sclPool.contains(e))
		{
			local = (glm::mat4)sclPool.getComponent(e);
			if (rotPool.contains(e)) local = rotPool.getComponent(e) * local;
			if (posPool.contains(e)) local = posPool.getComponent(e) * local;
		}
		else if (rotPool.contains(e))
		{
			local = (glm::mat4)rotPool.getComponent(e);
			if (posPool.contains(e)) local = posPool.getComponent(e) * local;
		}
		else if (posPool.contains(e)) 
		{
			local = (glm::mat4)rotPool.getComponent(e);
		}
	}
}

void updateTransform(Scene& scene) {
	using namespace Gawr::ECS;
	using namespace Transform;
	
	// update root transform
	{
		auto pipeline = scene.pipeline<World, const Local, const Parent, const UpdateTag>();
		for (auto [world, local] : pipeline.view<Select<World, const Local>, From<UpdateTag>, Where<AllOf<Parent>>>()) 
			// explicit Filter
			// Update tag implies it has Local Matrix and therfore World Matrix therefore no need to add include filter
		{
			world = (glm::mat4)local;
		}
	}

	// update branch transform
	{
		auto pipeline = scene.pipeline<const Entity, World, const Local, const Parent, UpdateTag>();
		auto& updatePool = pipeline.pool<UpdateTag>();

		for (auto [curr, parent, local, world] : pipeline.view<Select<Entity, const Parent, const Local, World>>())
		{
			// if parent updated
			if (updatePool.contains(parent))
			{
				// add update tag to current
				if (!updatePool.contains(curr)) updatePool.emplace(curr);

				// update curr by parent transform
				world = pipeline.pool<World>().getComponent(parent) * local;
			}
			// if current updated
			else if (updatePool.contains(curr))
			{
				// if parent has transform
				if (pipeline.pool<World>().contains(parent))
					// update curr by parent transform
					world = pipeline.pool<World>().getComponent(parent) * local;
				else
					// copy local matrix
					world = (glm::mat4)local;
			}
		}
	}

}

/*
// hybrid matrix quat technique
void updateTransform2(Scene& registry) {
	using namespace Gawr::ECS;
	using namespace Transform;
	// propergate update tag
	{
		auto pipeline = registry.pipeline<UpdateTag, const Parent>();

		for (auto [e, parent] : pipeline.view<Entity, const Parent>(Exclude<UpdateTag>{}))
		{
			if (pipeline.pool<UpdateTag>().contains(parent))
				pipeline.pool<UpdateTag>().emplace(e);
		}
	}

	auto updateScale = std::thread([&]
	{
		auto pipeline = registry.pipeline<const UpdateTag, const Parent, const Scale, WorldScale>();

		for (auto [local, world] : pipeline.view<
			Select<const Scale, WorldScale>,
			From<UpdateTag>,
			Exclude<Parent>>())
		{
			world = (glm::vec3)local;
		}

		for (auto [e, parent, world] : pipeline.view<
			Select<Entity, const Parent, WorldScale>,
			From<Parent>,
			Include<Parent, WorldScale, UpdateTag>>())
		{
			if (pipeline.pool<const Scale>().contains(e))
			{
				if (pipeline.pool<const WorldScale>().contains(parent))
					world = (const glm::vec3&)pipeline.pool<const WorldScale>().getComponent(parent) *
							(const glm::vec3&)pipeline.pool<const WorldScale>().getComponent(e);
				else
					world = (glm::vec3)pipeline.pool<const WorldScale>().getComponent(e);
			}
			else
			{
				if (pipeline.pool<const WorldScale>().contains(parent))
					world = pipeline.pool<const WorldScale>().getComponent(parent);
				//else
				//	pipeline.pool<WorldScale>().erase(e); // if current doesnt have rotation and neither does parent
			}
		}
	});

	auto updateRotation = std::thread([&]
	{
		auto pipeline = registry.pipeline<const UpdateTag, const Parent, const Rotation, WorldRotation>();

		for (auto [local, world] : pipeline.view<
			Select<const Rotation, WorldRotation>,
			From<UpdateTag>,
			Exclude<Parent>>())
		{
			world = (glm::quat)local;
		}

		for (auto [e, parent, local, world] : pipeline.view<
			Select<Entity, const Parent, Rotation, WorldRotation>,
			From<Parent>,
			Include<UpdateTag>>())
		{
			if (pipeline.pool<const WorldRotation>().contains(parent))
				world = (const glm::quat&)pipeline.pool<const WorldRotation>().getComponent(parent) * (const glm::quat&)local;
			else
				world = (glm::quat)local;
		}
	});

	updateScale.join();
	updateRotation.join();

	auto updatePosition = std::thread([&] {
		auto pipeline = registry.pipeline<const UpdateTag, const Parent, const Rotation, WorldRotation>();

		for (auto [local, world] : pipeline.view<
		Select<const Rotation, WorldRotation>,
			From<UpdateTag>,
			Exclude<Parent>>())
		{
			world = (glm::quat)local;
		}
	});


	updatePosition.join();
}

*/
