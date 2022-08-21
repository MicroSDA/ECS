#pragma once
#include "SparseSet.h"

namespace ecs
{
	/* Base components storage class */
	template<typename Entity>
	class Storage : public SparseSet<Entity>
	{
		friend class EntityManager;
	public:
		Storage(const TypeID& id, const TypeHash& hash, void(*destroy)(const Entity&, Storage<Entity>*)) :
			m_Id(id), m_Hash(hash), m_Destroy(destroy) {}
		virtual ~Storage() = default;
	public:
		TypeID GetId() const { return m_Id; }
		TypeID GetHash() const { return m_Hash; }
	protected:
		const TypeID											m_Id;
		const TypeHash											m_Hash;
		/* Destroy callback for single entity */
		void (*m_Destroy)(const Entity&, Storage<Entity>*)		= nullptr;
	};
	/* Component storage class */
	template<typename ComponentType, typename Entity>
	class ComponentStorage : public Storage<Entity>
	{
	private:
		static_assert(std::is_move_constructible_v<ComponentType>&& std::is_move_assignable_v<ComponentType>, "The managed type must be at least move constructible and assignable");
		/* Getting acces to storage class */
		using SetTraits = SparseSet<Entity>;
		using StorageTraits = Storage<Entity>;
	public:
		ComponentStorage() : Storage<Entity>(TypeInfo<ComponentType>::ID(), TypeInfo<ComponentType>::Hash(),
			[](const Entity& entity, Storage<Entity>* storage)
			{	/* Capture type */
				static_cast<ComponentStorage<ComponentType, Entity>*>(storage)->Remove(entity);
			}){}
		virtual ~ComponentStorage() = default; // TODO !
	public:
		/* Link component with given id */
		template<typename... Args>
		ComponentType& Add(const Entity& entity, Args&&... args)
		{
			assert(!Contains(entity) && "Entity has the component !");
			m_Components.emplace_back(std::make_unique<ComponentType>(std::forward<Args>(args)...));
			SetTraits::Push(entity);
			return *m_Components.back().get();
		}
		/* Unlink component from given id */
		void Remove(const Entity& entity)
		{
			assert(Contains(entity) && "Entity doesn't have the component !");
			auto other = std::move(m_Components.back());
			m_Components[SetTraits::GetPosition(entity)] = std::move(other);
			m_Components.pop_back();
			SetTraits::Pop(entity);
		}
		/* Get component which linked with given id */
		ComponentType& Get(const Entity& entity)
		{
			assert(Contains(entity) && "Entity doesn't have the component !");
			return *m_Components[SetTraits::GetPosition(entity)].get();
		}
		/* Return true if id is in storage */
		bool Contains(const Entity& entity) const { return SetTraits::Contains(entity); }
	private:
		std::vector<std::unique_ptr<ComponentType>> m_Components;
	};
}