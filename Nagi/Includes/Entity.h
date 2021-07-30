#pragma once
#include <entt.hpp>

namespace Nagi
{
	class Entity
	{
	public:
		Entity(entt::registry& registry, entt::entity id) : m_registry(registry), m_enttID(id) { }
		~Entity() {}

		template<typename T, typename... Args>
		void addComponent(Args &&... args)
		{
			// Only allow one component of each type
			if (m_registry.any_of<T>(m_enttID))
				return;

			m_registry.emplace<T>(m_enttID, std::forward<Args>(args)...);
		}

		template<typename T>
		void removeComponent()
		{
			m_registry.remove_if_exists<T>(m_enttID);
		}

		template<typename T>
		T& getComponent()
		{
			if (!m_registry.any_of<T>(m_enttID))
				assert(false);

			return m_registry.get<T>(m_enttID);
		}

	private:
		entt::registry& m_registry;
		entt::entity m_enttID;

		friend class Scene;


	};
}


