#include "ECS.h"
#include <iostream>

ecs::Entity::Entity(const EntityID& handle, ecs::EntityManager* manager):
	m_Handle(handle), m_Manager(manager)
{
}

bool ecs::Entity::IsValid() const
{
	return (m_Manager != nullptr && m_Manager->IsValidEntity(m_Handle));
}

ecs::EntityID ecs::Entity::GetID() const
{
	return EntityTraits<EntityID>::ToID(m_Handle);
}

void ecs::Entity::Destroy()
{
	assert(IsValid() && " Entity isn't valid !");
	UnsetParent();

	if (HasChildren())
	{
		for (auto& child : *this)
		{
			m_Manager->RemoveChild(*this, child);
			m_Manager->SetParent(child, ecs::null);
		}		
	}

	m_Manager->DestroyEntity(*this);
	m_Handle	= ecs::null;
	m_Manager	= nullptr;
}

void ecs::Entity::DestroyWithChildren()
{
	assert(IsValid() && " Entity isn't valid !");
	for (auto child : *this)
		child.DestroyWithChildren();

	Destroy();
}

void ecs::Entity::AddChild(Entity& child)
{
	assert(IsValid() && child.IsValid() && " Entity isn't valid !");
	assert(*this != child   && "Couldn't add child, parent and child are the same!");
	assert(!IsChildOf(child) && "Couldn't add child, parent and child allready linked!");
	assert(!m_Manager->HasParent(child) && "Couldn't add child, has allready parent!");

	if (!IsChildOf(child) && ! m_Manager->HasParent(child))
	{
		m_Manager->AddChild(*this, child);
		child.SetParent(*this);
	}
}

void ecs::Entity::RemoveChild(Entity& child)
{
	assert(IsValid() && child.IsValid() && " Entity isn't valid !");
	assert(*this != child && "Couldn't add child, parent and child are the same!");

	if (m_Manager->RemoveChild(*this, child))
		child.UnsetParent(true);
	else assert(false && "Child isn't part of current entitie!");
}

void ecs::Entity::SetParent(Entity& parent)
{
	assert(IsValid() && " Entity isn't valid !");
	if (HasParent())
	{
		m_Manager->RemoveChild(parent, *this);
	}
	m_Manager->SetParent(*this, parent);
}

void ecs::Entity::UnsetParent(const bool& isRecursive)
{
	assert(IsValid() && " Entity isn't valid !");
	auto parent = m_Manager->GetParent(*this);
	if (HasParent() && !isRecursive)
	{
		m_Manager->RemoveChild(parent, *this);
	}
	m_Manager->SetParent(*this, ecs::null);
}

bool ecs::Entity::HasChildren() const
{
	assert(IsValid() && " Entity isn't valid !");
	return m_Manager->HasChildren(m_Handle);
}

bool ecs::Entity::HasParent() const
{
	assert(IsValid() && " Entity isn't valid !");
	return m_Manager->HasParent(*this);
}

bool ecs::Entity::IsChildOf(const Entity& parent) const
{
	if (HasParent())
	{
		if (GetParent() == parent)
			return true;

		if (GetParent().IsChildOf(parent))
			return true;
	}
	return false;
}

ecs::Entity ecs::Entity::GetParent() const
{
	assert(IsValid() && " Entity isn't valid !");
	return { m_Manager->GetParent(m_Handle), m_Manager };
}

std::vector<ecs::EntityID>& ecs::Entity::GetChildren()
{
	assert(IsValid() && " Entity isn't valid !");
	const auto position = EntityTraits<EntityID>::ToID(*this);
	return std::get<2>(m_Manager->m_Entities[position]);
}

const std::vector<ecs::EntityID>& ecs::Entity::GetChildren() const
{
	assert(IsValid() && " Entity isn't valid !");
	const auto position = EntityTraits<EntityID>::ToID(*this);
	return std::get<2>(m_Manager->m_Entities[position]);
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
	if (OnEntityCreate)
		OnEntityCreate(entity);

	return entity;
}

void ecs::EntityManager::DestroyAllEntites()
{
	for (const EntityID& entityId : *this)
	{
		if (IsValidEntity(entityId))
		{
			Entity entity(entityId, this); entity.Destroy();
		}
	}
}

void ecs::EntityManager::SetOnEntityCreate(void(*onEntityCreate)(Entity&))
{
	OnEntityCreate = onEntityCreate;	
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
	for (auto position = m_Pools.size(); position; --position) {
		if (auto& pData = m_Pools[position - 1]; pData && pData->Contains(handle)) {
			pData->Destroy(handle, pData.get());
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
