#pragma once
#include "View.h"
#include "System.h"

namespace ecs
{
	class Entity;
	/* Entity manager, collect all entities */
	class EntityManager
	{
		friend class Entity;

		template<typename Entity, typename... Component>
		friend class BasicView;

		/* Entity manager iterator to iterate through all valid entities */
		template<typename Entity>
		class EntityManagerIterator
		{
		public:
			using iterator_category = std::random_access_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = Entity;
			using pointer = value_type*;
			using reference = value_type&;
		public:
			EntityManagerIterator(pointer first = nullptr, pointer last = nullptr, EntityManager* manager = nullptr) :
				m_First(first), m_Last(last), m_Current(first), m_Manager(manager)
			{
				/* Make sure that we are iterating only through valid entity*/
				if (m_Current != m_Last && !m_Manager->IsValidEntity(std::get<0>(*m_Current)))
					++(*this);
			}
		public:
			EntityManagerIterator& operator++(int) noexcept { while (++m_Current != m_Last && !m_Manager->IsValidEntity(std::get<0>(*m_Current))); return (*this); }
			EntityManagerIterator& operator--(int) noexcept { while (--m_Current != m_Last && !m_Manager->IsValidEntity(std::get<0>(*m_Current))); return (*this); }
			EntityManagerIterator& operator++() noexcept { while (++m_Current != m_Last && !m_Manager->IsValidEntity(std::get<0>(*m_Current))); return (*this); }
			EntityManagerIterator& operator--() noexcept { while (--m_Current != m_Last && !m_Manager->IsValidEntity(std::get<0>(*m_Current))); return (*this); }
			bool operator==(const EntityManagerIterator& other) const noexcept { return other.m_Current == m_Current; }
			bool operator!=(const EntityManagerIterator& other) const noexcept { return other.m_Current != m_Current; }
			EntityID& operator*() { return std::get<0>(*m_Current);; }
			EntityID* operator->() { return &std::get<0>(*m_Current); }
			const reference operator*() const { return *m_Current; }
			const pointer operator->() const { return m_Current; }
			operator bool() const { if (m_Current) return true; else return false; }
		private:
			pointer const m_First;
			pointer const m_Last;
			pointer m_Current;
			EntityManager* const m_Manager;
		};

		/* Manager */
	public:
		using Pools = std::vector<std::unique_ptr<Storage<EntityID>>>;
		using Systems = std::unordered_map<TypeID, std::unique_ptr<BasicSystem>>;
		using EntityData = std::tuple<EntityID, EntityID, std::vector<EntityID>>;
	private:
		using iterator = EntityManagerIterator<EntityData>;
		using const_iterator = EntityManagerIterator<const EntityData>;
	public:
		EntityManager() = default;
		EntityManager(void(*onCreateEntity)(Entity&));
		virtual ~EntityManager() = default;
	public:
		/* Begin of entities iterator */
		iterator begin() noexcept { return iterator(_EntitiesBegin(), _EntitiesEnd(), this); };
		/* End of entities iterator */
		iterator end() noexcept { return iterator(_EntitiesEnd(), _EntitiesEnd(), this); };
		/* Const begin of entities iterator */
		const_iterator cbegin() const noexcept { return const_iterator(_EntitiesBegin(), _EntitiesEnd(), const_cast<EntityManager*>(this)); };
		/* Const end of entities iterator */
		const_iterator cend() const noexcept { return const_iterator(_EntitiesEnd(), _EntitiesEnd(), const_cast<EntityManager*>(this)); };
	public:
		/* Create an entity */
		Entity CreateEntity();
		/* Destory all entities */
		void DestroyAllEntites();
		/* Set on entiti create callback function */
		void SetOnEntityCreate(void(*function)(Entity&));
		/* Return true if manager has give component pool */
		template<typename Component>
		const bool HasComponentPool() const { static const TypeID index = TypeInfo<Component>::ID(); return (m_Pools.size() > index); }
		/* Return view class that allow us to iterate through all entites with given set of components */
		template<typename... Component>
		BasicView<EntityID, Component...>View() { return BasicView<EntityID, Component...>(_GetCandidate<EntityID, Component...>(), &m_Pools, this); }
		template<typename Component>
		void RegisterSystem(void(*onCreate)(Component&), void(*onUpdate)(Component&), void(*onDestroy)(Component&))
		{
			static const TypeID index = TypeInfo<Component>::ID();
			m_Systems[index] = std::make_unique<System<Component>>(onCreate, onUpdate, onDestroy);
		}
		/* Return count of valid entities */
		std::size_t EntitiesCount() const;
	private:
		/* Add component to entity */
		template<typename Component, typename... Args>
		Component& AddComponent(const EntityID& entity, Args&&... args)
		{
			static const TypeID index = TypeInfo<Component>::ID();
			const auto handle = EntityTraits<EntityID>::ToID(entity);
			if (!HasComponentPool<Component>())
				m_Pools.resize(index + 1u);
			if (m_Pools[index] == nullptr)
				m_Pools[index] = std::make_unique<ComponentStorage<Component, EntityID>>();

			auto& component = static_cast<ComponentStorage<Component, EntityID>*>(m_Pools[index].get())->Add(handle, std::forward<Args>(args)...);
			if (m_Systems.find(index) != m_Systems.end())
				static_cast<System<Component>*>(m_Systems[index].get())->OnCreate(component);

			return component;
		}
		/* Get component from entity */
		template<typename Component>
		Component& GetComponent(const EntityID& entity)
		{
			assert(HasComponentPool<Component>() && "Entity doesn't have the component !");
			const auto handle = EntityTraits<EntityID>::ToID(entity);
			static const TypeID index = TypeInfo<Component>::ID();

			return static_cast<ComponentStorage<Component, EntityID>*>(m_Pools[index].get())->Get(handle);
		}
		/* Remove component from entity */
		template<typename Component>
		void RemoveComponent(const EntityID& entity)
		{
			assert(HasComponentPool<Component>() && "Entity doesn't have the component !");
			const auto handle = EntityTraits<EntityID>::ToID(entity);
			static const TypeID index = TypeInfo<Component>::ID();

			auto& component = GetComponent<Component>(handle);
			if (m_Systems.find(index) != m_Systems.end())
				static_cast<System<Component>*>(m_Systems[index].get())->OnDestroy(component);
			static_cast<ComponentStorage<Component, EntityID>*>(m_Pools[index].get())->Remove(handle);
		}
		/* Return true if entiti has give component */
		template<typename Component>
		bool HasComponent(const EntityID& entity) const
		{
			const auto handle = EntityTraits<EntityID>::ToID(entity);
			static const TypeID index = TypeInfo<Component>::ID();
			return (HasComponentPool<Component>() && static_cast<ComponentStorage<Component, EntityID>*>(m_Pools[index].get())->Contains(handle));
		}
		/* Return true if entity is valid */
		bool IsValidEntity(const EntityID& entity) const;
		/* Destory entity */
		void DestroyEntity(const EntityID& entity);
		/* Add child to entity */
		void AddChild(const EntityID& entity, const EntityID& child);
		/* Remove child from entity */
		bool RemoveChild(const EntityID& entity, const EntityID& child);
		/* Return true if entity has children */
		bool HasChildren(const EntityID& entity) const;
		/* Return parent handle*/
		EntityID GetParent(const EntityID& entity) const;
		/* Return true if entity has parent */
		bool HasParent(const EntityID& entity);
		/* Set parent for entity */
		void SetParent(const EntityID& entity, const EntityID& parent);
	private:
		const EntityData* _EntitiesBegin() const noexcept;
		const EntityData* _EntitiesEnd() const noexcept;
		EntityData* _EntitiesBegin() noexcept;
		EntityData* _EntitiesEnd() noexcept;
		/* Return lowest SparseSet or nullptr */
		template<typename Entity, typename... Component>
		const SparseSet<Entity>* _GetCandidate() const
		{
			if ((HasComponentPool<Component>() && ...))
			{
				return (std::min)({ static_cast<const SparseSet<Entity>*>(m_Pools[TypeInfo<Component>::ID()].get())... }, [](const auto& left, const auto& right)
					{
						return left->GetSize() < right->GetSize();
					});
			}
			return static_cast<SparseSet<Entity>*>(nullptr);
		}
	private:
		Pools m_Pools;
		Systems m_Systems;
		EntityID m_Destroyed = ecs::null;
		std::vector<EntityData>	m_Entities;
		void (*m_OnEntityCreate)(Entity&) = nullptr;
	private:
	};
}