#pragma once
#include "../Scene.h"

struct Parent {
	Gawr::ECS::Entity m_parent;
	operator Gawr::ECS::Entity() const { return m_parent; }

};

namespace Transform {
	struct WorldMatrix {
		glm::mat4 m_matrix;
		WorldMatrix(const glm::mat4& m) : m_matrix(m) { }
		operator const glm::mat4& () const { return m_matrix; }
	};
	struct LocalMatrix {
		glm::mat4 m_matrix;
		LocalMatrix(const glm::mat4& m) : m_matrix(m) { }
		operator const glm::mat4& () const { return m_matrix; }
	};
	struct Position {
		glm::vec3 m_position;
		operator const glm::vec3& () const { return m_position; }
		operator const glm::mat4() const { return glm::translate(m_position); }
	};
	struct Rotation {
		glm::quat m_rotation;
		operator const glm::quat& () const { return m_rotation; }
		operator const glm::mat4() const { return glm::mat4_cast(m_rotation); }
	};
	struct Scale {
		glm::vec3 m_scale;
		operator const glm::vec3& () const { return m_scale; }
		operator const glm::mat4() const { return glm::scale(m_scale); }
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
	class VBO;

	class VAO;
}

using Scene = Gawr::ECS::Registry<
	Gawr::ECS::Entity, // entity pool
	Parent, 
	Transform::Position, 
	Transform::Rotation, 
	Transform::Scale, 
	Transform::WorldMatrix,
	Transform::LocalMatrix,
	Transform::UpdateTag,
	Mesh::VAO,
	Mesh::VBO<Mesh::Attrib::Index>,
	Mesh::VBO<Mesh::Attrib::Position>,
	Mesh::VBO<Mesh::Attrib::Normal>,
	Mesh::VBO<Mesh::Attrib::Tangent>,
	Mesh::VBO<Mesh::Attrib::TexCoord>,
	Mesh::VBO<Mesh::Attrib::MaterialIndex>
>;



void updateHierarchy(Scene& registry) {
	using namespace Gawr::ECS;

	auto pipeline = registry.pipeline<const Entity, Parent>();

	auto& parentPool = pipeline.pool<Parent>();
	auto& entityPool = pipeline.pool<const Entity>();

	for (auto it = parentPool.rbegin(); it != parentPool.rend();) {	// iterate over Hierarchies in reverse
		Parent& parent = parentPool.getComponent(*it);				// parent to current entity

		if (!entityPool.valid(parent)) {	// if parent invalid
			parentPool.erase(*it);			// remove parent (does not capture all error case)
			continue;
		}
						
		if (parentPool.contains(parent))					// if parent has parent
		{
			Parent& next = parentPool.getComponent(parent);	// current entity's parent's parent 

			if (&parent > &next) {							// if parent appears before next parent
				parentPool.swap(*it, parent);				// swap their order to make parent appear first
				continue;
			}
		}

		it++;		
	}
}


#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <gtx/transform.hpp>

void updateLocalTransform(Scene& registry) {
	using namespace Gawr::ECS;
	using namespace Transform;
	auto pipeline = registry.pipeline<LocalMatrix, const UpdateTag, const Position, const Rotation, const Scale>();
	auto& posPool = pipeline.pool<const Position>();
	auto& rotPool = pipeline.pool<const Rotation>();
	auto& sclPool = pipeline.pool<const Scale>();

	for (auto [e, local] : pipeline.view<LocalMatrix, const UpdateTag>()) {

		if (sclPool.contains(e))
		{
			local = (glm::mat4)sclPool.getComponent(e);

			if (rotPool.contains(e))
				local = (glm::mat4)rotPool.getComponent(e) * (glm::mat4)local;

			if (posPool.contains(e))
				local = (glm::mat4)posPool.getComponent(e) * (glm::mat4)local;
		}
		else if (rotPool.contains(e))
		{
			local = (glm::mat4)rotPool.getComponent(e);

			if (posPool.contains(e))
				local = (glm::mat4)posPool.getComponent(e) * (glm::mat4)local;
		}
		else if (posPool.contains(e)) {
			local = (glm::mat4)posPool.getComponent(e);		
		}
	}
}

void updateRootTransform(Scene& registry) {
	using namespace Gawr::ECS;
	using namespace Transform;

	auto pipeline = registry.pipeline<WorldMatrix, const LocalMatrix, const Parent>();
	
	for (auto [e, world, local] : pipeline.view<WorldMatrix, const LocalMatrix>(ExcludeFilter<Parent>{})) {
		world = (glm::mat4)local;
	}
}

void updateBranchTransform(Scene& registry) {
	using namespace Gawr::ECS;
	using namespace Transform;

	auto pipeline = registry.pipeline<WorldMatrix, const LocalMatrix, const Parent, const UpdateTag>();
	auto& updatePool = pipeline.pool<UpdateTag>();
	auto& matPool = pipeline.pool<WorldMatrix>();


	for (auto [curr, parent, local, world] : pipeline.view<Parent, Entity, const Parent, const LocalMatrix, WorldMatrix>())
	{
		// if parent updated
		if (updatePool.contains(parent))
		{
			// add update tag to current
			if (!updatePool.contains(curr)) updatePool.emplace(curr);
			
			// update curr by parent transform
			world = (glm::mat4)matPool.getComponent(parent) * (glm::mat4)local;
		}
		// if current updated
		else if (updatePool.contains(curr))
		{
			// if parent has transform
			if (matPool.contains(parent)) 
				// update curr by parent transform
				world = (glm::mat4)matPool.getComponent(parent) * (glm::mat4)local;
			else
				// copy local matrix
				world = (glm::mat4)local;
		}	
	}
}



