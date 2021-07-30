#pragma once
#include <entt.hpp>
#include "Entity.h"
#include "Component.h"



namespace Nagi
{
	class Scene
	{
	public:
		Scene() {}
		~Scene() { m_registry.clear(); }

		Entity createEntity();
		void removeEntity(Entity* e);

		entt::registry& getRegistry();

	protected:
		// Scene is meant to be derived from

	private:
		entt::registry m_registry;
		

	};
}


