#include "ECS.h"
#include <iostream>

void OnCreateStr(std::string& str)
{
	std::cout << "OnCreate str :" << str << "\n";
}
void OnUpdateStr(std::string& str)
{
	std::cout << "OnUpdate str :" << str << "\n";
}
void OnDestroyStr(std::string& str)
{
	std::cout << "OnDestroy str :" << str << "\n";
}

void OnCreateSize_t(std::size_t& str)
{
	std::cout << "OnCreate size_t:" << str << "\n";
}
void OnUpdateSize_t(std::size_t& str)
{
	std::cout << "OnUpdate size_t:" << str << "\n";
}
void OnDestroySize_t(std::size_t& str)
{
	std::cout << "OnDestroy size_t:" << str << "\n";
}

int main()
{

	ecs::EntityManager entityManager([](ecs::Entity& entity)
		{
			entity.AddComponent<std::string>("Entity: " + std::string(entity));
			entity.AddComponent<std::size_t>(entity);
		});

	entityManager.RegisterSystem<std::string>(OnCreateStr, OnUpdateStr, OnDestroyStr);
	entityManager.RegisterSystem<std::size_t>(OnCreateSize_t, OnUpdateSize_t, OnDestroySize_t);

	ecs::Entity entity1 = entityManager.CreateEntity();
	ecs::Entity entity2 = entityManager.CreateEntity();
	ecs::Entity entity3 = entityManager.CreateEntity();
	ecs::Entity entity4 = entityManager.CreateEntity();

	entity1.AddChild(entity2);
	entity1.AddChild(entity3);
	entity1.AddChild(entity4);

	entityManager.OnUpdateSystem<std::string>();
	entityManager.OnUpdateSystem<std::size_t>();

	entityManager.View<std::string, std::size_t>().Each([](ecs::Entity& entity, std::string& str, std::size_t& value)
		{
			std::cout << str << " value: " << value << "\n";

		});


	//entityManager.View<std::string>().Each([](ecs::Entity& entity, std::string& str)
	//	{
	//		//TODO need to rewrite sparse set iterator, and try iterate by index, see entt::sparse_set
	//		//entity.RemoveComponent<std::string>();
	//	});

	


	return 0;
}