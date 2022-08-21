#include "EntityManager.h"
#include "Entity.h"

ecs::EntityManager::EntityManager(void(*onCreateEntity)(Entity&))
{
	m_OnEntityCreate = onCreateEntity;
}

ecs::EntityManager::~EntityManager()
{
	DestroyAllEntites();
}

ecs::Entity ecs::EntityManager::CreateEntity()
{
	EntityTraits<EntityID>::EntityType handle;
	if (m_Destroyed == ecs::null)
	{
		handle = std::get<0>(m_Entities.emplace_back(std::tuple<EntityID, EntityID, std::vector<EntityID>>{ EntityTraits<EntityID>::EntityType(static_cast<EntityTraits<EntityID>::EntityType>(m_Entities.size())), ecs::null, {}}));
	}
	else
	{
		const auto current = EntityTraits<EntityID>::ToIntegral(m_Destroyed);
		const auto version = EntityTraits<EntityID>::ToIntegral(std::get<0>(m_Entities[current])) & (EntityTraits<EntityID>::VersionMask << EntityTraits<EntityID>::EntityShift);

		m_Destroyed = EntityTraits<EntityID>::EntityType(EntityTraits<EntityID>::ToIntegral(std::get<0>(m_Entities[current])) & EntityTraits<EntityID>::EntityMask);
		handle = std::get<0>(m_Entities[current]) = EntityTraits<EntityID>::EntityType(current | version);
		std::get<1>(m_Entities[current]) = ecs::null;
	}

	Entity entity = Entity(handle, this);
	if (m_OnEntityCreate)
		m_OnEntityCreate(entity);

	return entity;
}

void ecs::EntityManager::DestroyAllEntites()
{
	for (auto& entity : *this)
		DestroyEntity(entity);
}

void ecs::EntityManager::SetOnEntityCreate(void(*function)(Entity&))
{
	m_OnEntityCreate = function;
}

std::size_t ecs::EntityManager::EntitiesCount() const
{
	return (m_Destroyed == ecs::null) ? m_Entities.size() : m_Entities.size() - (EntityTraits<EntityID>::ToID(m_Destroyed) + 1);
}
 
bool ecs::EntityManager::IsValidEntity(const EntityID& entity) const
{
	const auto position = EntityTraits<EntityID>::ToID(entity);
	return (entity != ecs::null && position < m_Entities.size() && std::get<0>(m_Entities[position]) == entity);
}

void ecs::EntityManager::DestroyEntity(const EntityID& entity)
{
	/* Extract id */
	auto handle = EntityTraits<EntityID>::EntityType(entity) & EntityTraits<EntityID>::EntityMask;
	/* Extract version */
	auto version = EntityTraits<EntityID>::VersionType((EntityTraits<EntityID>::ToIntegral(handle) >> EntityTraits<EntityID>::EntityShift) + 1);
	/* Mark entity as destroyed */
	std::get<0>(m_Entities[handle]) = EntityTraits<EntityID>::EntityType(EntityTraits<EntityID>::ToIntegral(m_Destroyed) | (EntityTraits<EntityID>::ToIntegral(version) << EntityTraits<EntityID>::EntityShift));
	m_Destroyed = EntityTraits<EntityID>::EntityType(entity);
	/* Remove entity from all pools and destroy all related components */
	for (auto position = m_Pools.size(); position; --position)
	{
		if (auto& pData = m_Pools[position - 1]; pData && pData->Contains(handle)) 
		{
			auto system = m_Systems.find(pData->GetID());
			if (system != m_Systems.end())
				pData->m_Destroy(handle, pData.get(), system->second.get());
			else
				pData->m_Destroy(handle, pData.get(), nullptr);
		}
	}
}

void ecs::EntityManager::AddChild(const EntityID& entity, const EntityID& child)
{
	const auto position = EntityTraits<EntityID>::ToID(entity);
	std::get<2>(m_Entities[position]).emplace_back(child);
}

bool ecs::EntityManager::RemoveChild(const EntityID& entity, const EntityID& child)
{
	const auto position = EntityTraits<EntityID>::ToID(entity);
	auto it = std::find(std::get<2>(m_Entities[position]).begin(), std::get<2>(m_Entities[position]).end(), child);
	if (it != std::get<2>(m_Entities[position]).end())
	{
		std::get<2>(m_Entities[position]).erase(it);
		return true;
	}
	return false;
}

bool ecs::EntityManager::HasChildren(const EntityID& entity) const
{
	const auto position = EntityTraits<EntityID>::ToID(entity);
	return std::get<2>(m_Entities[position]).size();
}

ecs::EntityID ecs::EntityManager::GetParent(const EntityID& entity) const
{
	const auto position = EntityTraits<EntityID>::ToID(entity);
	return std::get<1>(m_Entities[position]);
}

bool ecs::EntityManager::HasParent(const EntityID& entity)
{
	const auto position = EntityTraits<EntityID>::ToID(entity);
	return std::get<1>(m_Entities[position]) != ecs::null;
}

void ecs::EntityManager::SetParent(const EntityID& entity, const EntityID& parent)
{
	const auto position = EntityTraits<EntityID>::ToID(entity);
	std::get<1>(m_Entities[position]) = parent;
}

const ecs::EntityManager::EntityData* ecs::EntityManager::_EntitiesBegin() const noexcept
{
	return m_Entities.data();
}

const ecs::EntityManager::EntityData* ecs::EntityManager::_EntitiesEnd() const noexcept
{
	return m_Entities.data() + m_Entities.size();
}

ecs::EntityManager::EntityData* ecs::EntityManager::_EntitiesBegin() noexcept
{
	return const_cast<ecs::EntityManager::EntityData*>(const_cast<const ecs::EntityManager*>(this)->_EntitiesBegin());
}

ecs::EntityManager::EntityData* ecs::EntityManager::_EntitiesEnd() noexcept
{
	return const_cast<ecs::EntityManager::EntityData*>(const_cast<const ecs::EntityManager*>(this)->_EntitiesEnd());
}
