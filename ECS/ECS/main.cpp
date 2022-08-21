#include "ECS.h"
#include <iostream>

void OnCreate(std::string& str)
{
	std::cout << "OnCreate :" << str << "\n";
}
void OnUpdate(std::string& str)
{
	std::cout << "OnUpdate :" << str << "\n";
}
void OnDestroy(std::string& str)
{
	std::cout << "OnDestroy :" << str << "\n";
}

int main()
{
	ecs::EntityManager entityManager([](ecs::Entity& entity) { entity.AddComponent<std::string>("Entity: " + std::string(entity)); });
	entityManager.RegisterSystem<std::string>(OnCreate, OnUpdate, OnDestroy);

	ecs::Entity entity1 = entityManager.CreateEntity();
	ecs::Entity entity2 = entityManager.CreateEntity();
	ecs::Entity entity3 = entityManager.CreateEntity();
	ecs::Entity entity4 = entityManager.CreateEntity();

	entity1.AddChild(entity2);
	entity1.AddChild(entity3);
	entity1.AddChild(entity4);

	entity1.RemoveComponent<std::string>();
	entity2.RemoveComponent<std::string>();
	entity3.RemoveComponent<std::string>();
	entity4.RemoveComponent<std::string>();

	entityManager.View<std::string>().Each([](ecs::Entity& entity, std::string& str)
		{
			//TODO need to rewrite sparse set iterator, and try iterate by index, see entt::sparse_set
			//entity.RemoveComponent<std::string>();
		});

	entityManager.View<std::string, std::size_t>().Each([](ecs::Entity& entity, std::string& str, std::size_t& value)
		{
			std::cout << str << " value: " << value << "\n";
			
		});


	return 0;
}