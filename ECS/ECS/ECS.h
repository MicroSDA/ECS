#pragma once
#include <vector>
#include <algorithm>
#include <functional>
#include <memory>
#include <cassert>
#include <array>
#include <string>
#include <type_traits>

namespace ecs
{
	/* Type identifier */
	using TypeID = std::size_t;
	/* Type hash value */
	using TypeHash = std::size_t;
	/* Entity identifier */
	using EntityID = std::size_t;
	/**********************************************/
	template<typename, typename = void>
	struct EntityTraits;
	template<>
	struct EntityTraits<std::uint32_t> 
	{
		using EntityType = std::uint32_t;
		using VersionType = std::uint16_t;
		using DifferenceType = std::int32_t;
		/* Mask to use to get the entity number out of an identifier. */
		static constexpr EntityType EntityMask = 0xFFFFF;
		/* Mask to use to get the version out of an identifier. */
		static constexpr EntityType VersionMask = 0xFFF;
		/* Extent of the entity number within an identifier. */
		static constexpr std::size_t EntityShift = 20u;
		/* Convert to itegral */
		[[nodiscard]] static constexpr std::uint32_t ToIntegral(const std::uint32_t& value) noexcept
		{
			return static_cast<EntityTraits<std::uint32_t>::EntityType>(value);
		}
		/* Convert to index without version */
		[[nodiscard]] static constexpr std::uint32_t ToID(const std::uint32_t& value) noexcept
		{
			return static_cast<EntityTraits<std::uint32_t>::EntityType>(value) & EntityTraits<uint32_t>::EntityMask;
		}
	};
	template<>
	struct EntityTraits<std::uint64_t> 
	{
		using EntityType = std::uint64_t;
		using VersionType = std::uint32_t;
		using DifferenceType = std::int64_t;
		/* Mask to use to get the entity number out of an identifier. */
		static constexpr EntityType EntityMask = 0xFFFFFFFF;
		/* Mask to use to get the version out of an identifier. */
		static constexpr EntityType VersionMask = 0xFFFFFFFF;
		/* Extent of the entity number within an identifier. */
		static constexpr std::size_t EntityShift = 32u;
		/* Convert to itegral */
		[[nodiscard]] static constexpr std::uint64_t ToIntegral(const std::uint64_t& value) noexcept
		{
			return static_cast<EntityTraits<std::uint64_t>::EntityType>(value);
		}
		/* Convert to index without version */
		[[nodiscard]] static constexpr std::uint64_t ToID(const std::uint64_t& value) noexcept
		{
			return static_cast<EntityTraits<std::uint64_t>::EntityType>(value) & EntityTraits<uint64_t>::EntityMask;
		}
	};
	/* Null value for entity (unsigned - 1)*/
	struct Null
	{
		template<typename Entity>
		[[nodiscard]] constexpr operator Entity() const noexcept
		{
			return Entity(EntityTraits<Entity>::EntityMask);
		}
		[[nodiscard]] constexpr bool operator==(Null) const noexcept
		{
			return true;
		}
		[[nodiscard]] constexpr bool operator!=(Null) const noexcept
		{
			return false;
		}
		template<typename Entity>
		[[nodiscard]] constexpr bool operator==(const Entity entity) const noexcept 
		{
			return (EntityTraits<Entity>::ToIntegral(entity) & EntityTraits<Entity>::EntityMask) == EntityTraits<Entity>::ToIntegral(static_cast<Entity>(*this));
		}
		template<typename Entity>
		[[nodiscard]] constexpr bool operator!=(const Entity entity) const noexcept 
		{
			return !(entity == *this);
		}
	};
	template<typename Entity>
	[[nodiscard]] constexpr bool operator==(const Entity entity, Null other) noexcept 
	{
		return other.operator==(entity);
	}
	template<typename Entity>
	[[nodiscard]] constexpr bool operator!=(const Entity entity, Null other) noexcept
	{
		return !(other == entity);
	}
	inline constexpr Null null{};
	
	namespace internal
	{
		struct TypeInfo
		{
			[[nodiscard]] static TypeID Next() noexcept
			{
				static TypeID value = 0u;
				return value++;
			}
		};
	}
	template<typename Type>
	struct TypeInfo
	{
		/* Get index id of specific type */
		[[nodiscard]] static TypeID ID() noexcept
		{
			static const TypeID value = internal::TypeInfo::Next();
			return value;
		}
		/* Get hash value of specific type */
		[[nodiscard]] static TypeHash Hash() noexcept
		{
			static const TypeHash value = TypeHash(typeid(Type).hash_code());
			return value;
		}
		/* Get name of specific type */
		[[nodiscard]] static const char* Name() noexcept
		{
			static const auto value = typeid(Type).name();
			return value;
		}
	};

	/* Sparse set */
	template<typename Type>
	class SparseSet
	{
	public:
		template<typename Type>
		class Iterator
		{
		public:
			Iterator(const Type* first = nullptr, const Type* last = nullptr) :
				m_First(first), m_Current(const_cast<Type*>(first)), m_Last(last) {}
			~Iterator() = default;

			Iterator<Type>& operator++() { ++m_Current; return (*this); }
			Iterator<Type>& operator--() { --m_Current; return (*this); }
			bool			operator==(const Iterator<Type>& ptr)const { return (m_Current == ptr.m_Current); }
			bool			operator!=(const Iterator<Type>& ptr)const { return (m_Current != ptr.m_Current); }
			Type&			operator*() { return *m_Current; }
			Type*			operator->() { return m_Current; }
			const Type&		operator*() const { return *m_Current; }
			operator bool() const { if (m_Current) return true; else return false; }
		private:
			const Type* m_First;
			const Type* m_Last;
			Type*		m_Current;
		};
		/* Iterators */
		Iterator<Type>			begin()			{ return Iterator<Type>(m_Packed.data(), m_Packed.data() + m_Packed.size()); }
		Iterator<Type>			end()			{ return Iterator<Type>(m_Packed.data() + m_Packed.size(), m_Packed.data() + m_Packed.size()); }
		Iterator<const Type>	cbegin()  const { return Iterator<const Type>(m_Packed.data(), m_Packed.data() + m_Packed.size()); }
		Iterator<const Type>	cend()	  const { return Iterator<const Type>(m_Packed.data() + m_Packed.size(), m_Packed.data() + m_Packed.size()); }
	public:
		inline void Push(const Type& value) 
		{
			const auto position = m_Packed.size();
			m_Packed.push_back(value);
			if (!(value < m_Sparse.size()))
				m_Sparse.resize(value + 1);
			m_Sparse[value] = position;
		}
		void Pop(const Type& value)
		{
			const auto last = m_Packed.back();
			std::swap(m_Packed.back(), m_Packed[m_Sparse[value]]);
			std::swap(m_Sparse[last], m_Sparse[value]);
			m_Packed.pop_back();
		}
		std::size_t GetPosition(const Type& value) const { return m_Sparse[value]; }
		const bool Contains(const Type& value) const { return (value < m_Sparse.size() && m_Sparse[value] < m_Packed.size() && m_Packed[m_Sparse[value]] == value); }
		void Sort()
		{
			std::sort(m_Packed.begin(), m_Packed.end());
			for (auto position = 0; position < m_Packed.size(); position++) {
				m_Sparse[m_Packed[position]] = position;
			}
		}
		std::size_t GetSize() const { return m_Packed.size(); }
		const Type* GetData() const { return m_Packed.data(); }
	private:
		std::vector<Type> m_Sparse;
		std::vector<Type> m_Packed;
	};

	/* Storage */
	template<typename Entity>
	class Storage : public SparseSet<Entity>
	{
	public:
		Storage(const TypeID& id, const TypeHash& hash): m_Id(id), m_Hash(hash) {}
		virtual ~Storage() = default;
		std::function<void(const Entity&, Storage*)>	        Destroy;
	protected:
		const TypeID											m_Id;
		const TypeHash											m_Hash;
	};
	template<typename ComponentType, typename Entity>
	class ComponentStorage : public Storage<Entity>
	{
	private:
		static_assert(std::is_move_constructible_v<ComponentType>&& std::is_move_assignable_v<ComponentType>, "The managed type must be at least move constructible and assignable");
		/* Getting acces to storage class */
		using _Set		= SparseSet<Entity>;
		using _Storage	= Storage<Entity>;
	public:
		ComponentStorage() : Storage<Entity>(TypeInfo<ComponentType>::ID(), TypeInfo<ComponentType>::Hash())
		{
			/* Create destroy function for a single component !*/
			this->Destroy = [](const Entity& entity, Storage<Entity>* storage)
			{
				static_cast<ComponentStorage<ComponentType, Entity>*>(storage)->Remove(entity);
			};
		}
		virtual ~ComponentStorage() = default; // TODO !
	public:
		/* Link component with given id */
		template<typename... Args>
		ComponentType& Add(const Entity& entity, Args&&... args)
		{
			assert(!Contains(entity) && "Entity has the component !");
			m_Components.emplace_back(std::make_unique<ComponentType>(std::forward<Args>(args)...));
			_Set::Push(entity);
			return *m_Components.back().get();
		}
		void Remove(const Entity& entity)
		{
			assert(Contains(entity) && "Entity doesn't have the component !");
			auto other = std::move(m_Components.back());

			m_Components[_Set::GetPosition(entity)] = std::move(other);
			m_Components.pop_back();
			_Set::Pop(entity);
		}
		ComponentType& Get(const Entity& entity)
		{
			assert(Contains(entity) && "Entity doesn't have the component !");
			return *m_Components[_Set::GetPosition(entity)].get();
		}
		bool Contains(const Entity& entity) const
		{
			return _Set::Contains(entity);
		}
	private:
		std::vector<std::unique_ptr<ComponentType>> m_Components;
	};

	class Entity;
	class EntityManager;

	template<typename Entity, typename... Components>
	class ComponentView
	{
	public:
		using Others = std::array<const SparseSet<Entity>*, (sizeof...(Components) - 1)>;
		template<typename Type>
		class Iterator
		{
		public:
			Iterator(const Type* first = nullptr, const Type* last = nullptr, Others others = {}) :
				m_First(first), m_Current(const_cast<Type*>(first)), m_Last(last), m_Others(others)
			{
				if (m_Current != m_Last && !InOthers())
					++(*this);
			}
			~Iterator() = default;

			Iterator<Type>& operator++() { while (++m_Current != m_Last && !InOthers()); return (*this); }
			Iterator<Type>& operator--() { while (--m_Current != m_First && !InOthers()); return (*this); }
			bool			operator==(const Iterator<Type>&ptr)const { return (m_Current == ptr.m_Current); }
			bool			operator!=(const Iterator<Type>&ptr)const { return (m_Current != ptr.m_Current); }
			Type& operator*() { return *m_Current; }
			Type* operator->() { return m_Current; }
			const Type& operator*() const { return *m_Current; }
			operator bool() const { if (m_Current) return true; else return false; }
		private:
			const Type* m_First;
			const Type* m_Last;
			Type*		m_Current;
			Others		m_Others;
		private:
			[[nodiscard]] bool InOthers() const
			{
				return std::all_of(m_Others.cbegin(), m_Others.cend(), [entt = *m_Current](const SparseSet<Entity>* current) { return current->Contains(entt); });
			}
		};
	public:
		ComponentView(const SparseSet<Entity>* candidate, const std::vector<std::unique_ptr<Storage<Entity>>>* pools, const EntityManager* manager) :
			m_Candidate(candidate), m_Pools(pools), m_Manager(manager),
			m_ItFirst((candidate&& candidate->GetSize()) ? candidate->GetData() : nullptr),
			m_ItLast((candidate&& candidate->GetSize()) ? candidate->GetData() + candidate->GetSize() : nullptr)
		{};
		template<typename Function>
		void Each(Function function)
		{
			for (auto entity : *this)
				function({ entity, const_cast<EntityManager*>(m_Manager) }, const_cast<EntityManager*>(m_Manager)->GetComponent<Components>(entity)...);
		}
	public:
		Iterator<Entity>		begin()			{ return Iterator<Entity>(m_ItFirst, m_ItLast, _GetOthers(m_Candidate, *m_Pools)); }
		Iterator<Entity>		end()			{ return Iterator<Entity>(m_ItLast, m_ItLast, _GetOthers(m_Candidate, *m_Pools)); }
		Iterator<const Entity>	cbegin() const	{ return Iterator<const Entity>(m_ItFirst, m_ItLast, _GetOthers(m_Candidate, *m_Pools)); }
		Iterator<const Entity>	cend()	 const	{ return Iterator<const Entity>(m_ItLast, m_ItLast, _GetOthers(m_Candidate, *m_Pools)); }
	private:
		[[nodiscard]] Others _GetOthers(const SparseSet<Entity>* candidate, const std::vector<std::unique_ptr<Storage<Entity>>>& pools) const
		{
			std::size_t position = 0; Others others{};
			if (candidate)
				((static_cast<SparseSet<Entity>*>(pools[TypeInfo<Components>::ID()].get()) == candidate ? nullptr : (others[position] = static_cast<SparseSet<Entity>*>(pools[TypeInfo<Components>::ID()].get()), others[position++])), ...);
			return others;
		}
	private:
		const SparseSet<Entity>*	m_Candidate;
		const Entity*				m_ItFirst;
		const Entity*				m_ItLast;
		const EntityManager*		m_Manager;
		const std::vector<std::unique_ptr<Storage<Entity>>>* m_Pools;
	};
	/* Entity manager */
	class EntityManager
	{
		friend class Entity;
	public:
		using EntityData = std::tuple<EntityID, EntityID, std::vector<EntityID>>;

		template<typename Type>
		class Iterator
		{
		public:
			Iterator(const Type* first = nullptr, const Type* last = nullptr, const EntityManager* manager = nullptr) :
				m_First(first), m_Current(const_cast<Type*>(first)), m_Last(last), m_Manager(const_cast<EntityManager*>(manager))
			{
				if (m_Current != m_Last && !m_Manager->IsValidEntity(std::get<0>(*m_Current)))
					++(*this);
			}
			Iterator<Type>& operator++() { while (++m_Current != m_Last && !m_Manager->IsValidEntity(std::get<0>(*m_Current))); return (*this); }
			Iterator<Type>& operator--() { while (--m_Current != m_Last && !m_Manager->IsValidEntity(std::get<0>(*m_Current))); return (*this); }
			bool			operator==(const Iterator<Type>& ptr) const { return (m_Current == ptr.m_Current); }
			bool			operator!=(const Iterator<Type>& ptr) const { return (m_Current != ptr.m_Current); }
			EntityID& operator*()  { return  std::get<0>(*m_Current);  }
			EntityID* operator->() { return  &std::get<0>(*m_Current); }
			const Type& operator*() const { return *m_Current; }
			operator bool() const { if (m_Current) return true; else return false; }
		private:
			const Type*		m_First;
			const Type*		m_Last;
			Type*			m_Current;
			EntityManager*  m_Manager;
		};
	public:
		EntityManager() = default;
		virtual ~EntityManager() = default;
	public:
		Entity CreateEntity();
		void DestroyAllEntites();
		template<typename Component>
		const bool HasComponentPool() const
		{
			static const TypeID index = TypeInfo<Component>::ID();
			return (m_Pools.size() > index);
		}
		template<typename... Components>
		ComponentView<EntityID, Components...> View()
		{
			return ComponentView<EntityID, Components...>(_GetCandidate<EntityID, Components...>(), &m_Pools, this);
		}
		template<typename Component>
		Component& GetComponent(const EntityID& entity)
		{
			assert(HasComponentPool<Component>() && "Entity doesn't have the component !");
			const auto handle = EntityTraits<EntityID>::ToID(entity);
			static const TypeID index = TypeInfo<Component>::ID();

			return static_cast<ComponentStorage<Component, EntityID>*>(m_Pools[index].get())->Get(handle);
		}
		std::size_t EntitiesCount() const
		{
			return (m_Destroyed == ecs::null) ? m_Entities.size() : m_Entities.size() - (EntityTraits<EntityID>::ToID(m_Destroyed) + 1);
		}
		void SetOnEntityCreate(void (*onEntityCreate)(Entity&));
	public:
		Iterator<EntityData>			begin()			{ return Iterator<EntityData>(m_Entities.data(), m_Entities.data() + m_Entities.size(), this); }
		Iterator<EntityData>			end()			{ return Iterator<EntityData>(m_Entities.data() + m_Entities.size(), m_Entities.data() + m_Entities.size(), this); }
		Iterator<const EntityData>		cbegin() const  { return Iterator<const EntityData>(m_Entities.data(), m_Entities.data() + m_Entities.size(), this); }
		Iterator<const EntityData>		cend()	 const  { return Iterator<const EntityData>(m_Entities.data() + m_Entities.size(), m_Entities.data() + m_Entities.size(), this); }
	private:
		template<typename Component, typename... Args>
		Component& AddComponent(const EntityID& entity, Args&&... args)
		{
			static const TypeID index = TypeInfo<Component>::ID();
			const auto handle = EntityTraits<EntityID>::ToID(entity);
			if (!HasComponentPool<Component>())
				m_Pools.resize(index + 1u);
			if (m_Pools[index] == nullptr)
				m_Pools[index] = std::make_unique<ComponentStorage<Component, EntityID>>();

			return static_cast<ComponentStorage<Component, EntityID>*>(m_Pools[index].get())->Add(handle, std::forward<Args>(args)...);
		}
		template<typename Component>
		void RemoveComponent(const EntityID& entity)
		{
			assert(HasComponentPool<Component>() && "Entity doesn't have the component !");
			const auto handle = EntityTraits<EntityID>::ToID(entity);
			static const TypeID index = TypeInfo<Component>::ID();

			static_cast<ComponentStorage<Component, EntityID>*>(m_Pools[index].get())->Remove(handle);
		}
		template<typename Component>
		bool HasComponent(const EntityID& entity) const
		{
			const auto handle = EntityTraits<EntityID>::ToID(entity);
			static const TypeID index = TypeInfo<Component>::ID();
			return (HasComponentPool<Component>() && static_cast<ComponentStorage<Component, EntityID>*>(m_Pools[index].get())->Contains(handle));
		}
		bool IsValidEntity(const EntityID& entity) const
		{
			const auto position = EntityTraits<EntityID>::ToID(entity);
			return (entity != ecs::null && position < m_Entities.size() && std::get<0>(m_Entities[position]) == entity);
		}
		void DestroyEntity(const EntityID& entity);

		void AddChild(const EntityID& entity, const EntityID& child);
		bool RemoveChild(const EntityID& entity, const EntityID& child);
		bool HasChildren(const EntityID& entity) const;
		EntityID GetParent(const EntityID& entity) const;
		bool HasParent(const EntityID& entity);
		void SetParent(const EntityID& entity, const EntityID& parent);
	private:
		template<typename Entity, typename... Components>
		const SparseSet<Entity>* _GetCandidate() const
		{
			if ((HasComponentPool<Components>() && ...))
			{
				return (std::min)({ static_cast<const SparseSet<Entity>*>(m_Pools[TypeInfo<Components>::ID()].get())... }, [](const auto& left, const auto& right)
					{
						return left->GetSize() < right->GetSize();
					});
			}
			return static_cast<SparseSet<Entity>*>(nullptr);
		}
	private:
		std::vector<std::unique_ptr<Storage<EntityID>>>						m_Pools;
		std::vector<EntityData>												m_Entities;
		EntityID															m_Destroyed { ecs::null };
		void (*OnEntityCreate)(Entity&)										= nullptr;
	};
	class Entity
	{
	public:
		template<typename Type>
		class Iterator
		{
		public:
			Iterator(const Type* first = nullptr, const Type* last = nullptr, const EntityManager* manager = nullptr) :
				m_First(first), m_Current(const_cast<Type*>(first)), m_Last(last), m_Manager(const_cast<EntityManager*>(manager))
			{
				if (m_Current)
					m_Entity.m_Handle = *m_Current;
				m_Entity.m_Manager = m_Manager;
			}
			~Iterator() = default;
			Iterator<Type>& operator++() { m_Entity.m_Handle = *m_Current; ++m_Current; return (*this);}
			Iterator<Type>& operator--() { m_Entity.m_Handle = *m_Current; --m_Current; return (*this);}
			bool			operator==(const Iterator<Type>& ptr)const { return (m_Current == ptr.m_Current); }
			bool			operator!=(const Iterator<Type>& ptr)const { return (m_Current != ptr.m_Current); }
			Entity& operator*()  { return m_Entity; }
			Entity* operator->() { return &m_Entity; }
			const Type& operator*() const { return *m_Current; }
			operator bool() const { if (m_Current) return true; else return false; }
		private:
			const Type*		m_First;
			const Type*		m_Last;
			Type*			m_Current;
			Entity			m_Entity;
			EntityManager*  m_Manager;
		};

		Iterator<EntityID>				begin()			{ return Iterator<EntityID>(GetChildren().data(), GetChildren().data() + GetChildren().size(), m_Manager); }
		Iterator<EntityID>				end()			{ return Iterator<EntityID>(GetChildren().data() + GetChildren().size(), GetChildren().data() + GetChildren().size(), m_Manager); }
		Iterator<const EntityID>		cbegin() const	{ return Iterator<const EntityID>(GetChildren().data(), GetChildren().data() + GetChildren().size(), m_Manager); }
		Iterator<const EntityID>		cend()   const	{ return Iterator<const EntityID>(GetChildren().data() + GetChildren().size(), GetChildren().data() + GetChildren().size(), m_Manager); }
	public:
		Entity(const EntityID& handle = ecs::null, EntityManager* manager = nullptr);
		virtual ~Entity() = default;
	public:
		template<typename Component, typename... Args>
		Component& AddComponent(Args&&... args)
		{
			assert(IsValid() && " Entity isn't valid !");
			static const TypeID index = TypeInfo<Component>::ID();
			return m_Manager->AddComponent<Component>(m_Handle, std::forward<Args>(args)...);
		}
		template<typename Component>
		Component& GetComponent()
		{
			assert(IsValid() && " Entity isn't valid !");
			static const TypeID index = TypeInfo<Component>::ID();
			return m_Manager->GetComponent<Component>(m_Handle);
		}
		template<typename Component>
		void RemoveComponent()
		{
			assert(IsValid() && " Entity isn't valid !");
			static const TypeID index = TypeInfo<Component>::ID();
			m_Manager->RemoveComponent<Component>(m_Handle);
		}
		template<typename Component>
		const bool HasComponent() const
		{
			assert(IsValid() && " Entity isn't valid !");
			return m_Manager->HasComponent<Component>(m_Handle);
		}
		bool IsValid() const;
		EntityID GetID() const;
		void Destroy();
		void DestroyWithChildren();

		void AddChild(Entity& child);
		void RemoveChild(Entity& child);
		void SetParent(Entity& child);
		void UnsetParent(const bool& isRecursive = false);
		bool HasChildren() const;
		bool HasParent() const;
		bool IsChildOf(const Entity& parent) const;
		Entity GetParent() const;

		operator bool() const			{ return IsValid(); }
		operator EntityID() const		{ return GetID(); }
		operator int() const			{ return static_cast<int>(GetID()); }
		bool operator==(const Entity& other) const { return m_Handle == other.m_Handle && m_Manager == other.m_Manager; }
		bool operator!=(const Entity& other) const { return !(*this == other); }
		friend std::string operator+(const std::string& string, const Entity& other) { return std::string(string + std::to_string(static_cast<EntityID>(other.GetID()))); }
		operator std::string() { return std::to_string(static_cast<EntityID>(GetID())); }
		operator const std::string() const { return std::to_string(static_cast<EntityID>(GetID())); }

	private:
		std::vector<EntityID>& GetChildren();
		const std::vector<EntityID>& GetChildren() const;
	private:
		EntityID		m_Handle;
		EntityManager*  m_Manager;
	};
}