#include "pch.h"
#include "Scene.h"

namespace Nagi
{

	Entity Scene::createEntity()
	{
		Entity e = Entity(m_registry, m_registry.create());
		e.addComponent<TransformComponent>(0.f, 0.f, 0.f);
		return e;
	}

	void Scene::removeEntity(Entity* e)
	{
		m_registry.destroy(e->m_enttID);
	}

	entt::registry& Scene::getRegistry()
	{
		return m_registry;
	}

}
